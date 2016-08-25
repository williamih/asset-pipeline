#include "AssetPipeline.h"
#include <string>
#include <lua.hpp>
#include <Core/Macros.h>
#include "AssetPipelineOsFuncs.h"

const char* const BUILD_SCRIPT_RELATIVE_PATH = "assetpipeline.lua";

static const char* const BUILD_SYSTEM_SCRIPTS[] = {
    "list.lua",
    "buildsystem.lua",
};

AssetPipeline::AssetPipeline()
    : m_thread(&AssetPipeline::CompileProc, this)
    , m_nextDir()
    , m_mutex()
    , m_compileInProgress(false)
    , m_condVar()
{}

AssetPipeline::~AssetPipeline()
{
    m_thread.join();
}

void AssetPipeline::SetProjectWithDirectory(const char* path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nextDir = path;
}

static const char KEY_RULES = 0;
static const char KEY_TASKS = 0;
static const char KEY_CONTENTDIR = 0;

static int lua_Rule(lua_State* L)
{
    if (lua_gettop(L) != 2 || !lua_isstring(L, 1) || !lua_isfunction(L, 2))
        return luaL_error(L, "Usage: Rule(pattern, func)");

    lua_pushlightuserdata(L, (void*)&KEY_RULES);
    lua_gettable(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_settable(L, -3);

    return 0;
}

static int lua_Task(lua_State* L)
{
    if (lua_gettop(L) != 2 || !lua_isstring(L, 1) || !lua_istable(L, 2))
        return luaL_error(L, "Usage: Task(name, { Parse = func, Execute = func })");

    lua_pushlightuserdata(L, (void*)&KEY_TASKS);
    lua_gettable(L, LUA_REGISTRYINDEX);

    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_settable(L, -3);

    return 0;
}

static int lua_ContentDir(lua_State* L)
{
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1))
        return luaL_error(L, "Usage: ContentDir(path)");

    lua_pushlightuserdata(L, (void*)&KEY_CONTENTDIR);
    lua_pushvalue(L, 1);
    lua_settable(L, LUA_REGISTRYINDEX);

    return 0;
}

static int lua_ContentDirIterator_Closure(lua_State* L)
{
    using AssetPipelineOsFuncs::DirectoryIterator;

    if (lua_gettop(L) != 0)
        return luaL_error(L, "Expected 0 arguments, got %d", lua_gettop(L));

    DirectoryIterator** ptr = (DirectoryIterator**)lua_touserdata(
        L, lua_upvalueindex(1)
    );
    DirectoryIterator* iter = *ptr;

    std::pair<std::string, bool> result = iter->Next();
    if (result.second) {
        lua_pushlightuserdata(L, (void*)&KEY_CONTENTDIR);
        lua_gettable(L, LUA_REGISTRYINDEX);
        std::string path(lua_tostring(L, -1));
        lua_pop(L, 1);
        path = path + "/" + result.first;
        lua_pushstring(L, path.c_str());
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int lua_GetContentDirIterator(lua_State* L)
{
    using AssetPipelineOsFuncs::DirectoryIterator;

    if (lua_gettop(L) != 0)
        return luaL_error(L, "Usage: GetContentDirIterator()");

    lua_pushlightuserdata(L, (void*)&KEY_CONTENTDIR);
    lua_gettable(L, LUA_REGISTRYINDEX);
    DirectoryIterator* iter = DirectoryIterator::Create(lua_tostring(L, -1));
    lua_pop(L, 1);

    DirectoryIterator** ptr = (DirectoryIterator**)lua_newuserdata(
        L, sizeof(DirectoryIterator**)
    );
    *ptr = iter;

    lua_pushcclosure(L, lua_ContentDirIterator_Closure, 1);

    return 1;
}

static lua_State* SetupLuaState(const char* projectPath)
{
    lua_State* L = luaL_newstate();

    luaopen_base(L);
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);

    lua_pushlightuserdata(L, (void*)&KEY_RULES);
    lua_newtable(L);
    lua_settable(L, LUA_REGISTRYINDEX);

    lua_pushlightuserdata(L, (void*)&KEY_TASKS);
    lua_newtable(L);
    lua_settable(L, LUA_REGISTRYINDEX);

    lua_register(L, "Rule", lua_Rule);
    lua_register(L, "Task", lua_Task);
    lua_register(L, "ContentDir", lua_ContentDir);
    lua_register(L, "GetContentDirIterator", lua_GetContentDirIterator);

    int ret = luaL_dofile(L, BUILD_SCRIPT_RELATIVE_PATH);
    if (ret != 0) {
        DebugPrint("Failed to run build script for project at path: %s", projectPath);
        DebugPrint("Error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        exit(1);
    }

    std::string scriptsPath(AssetPipelineOsFuncs::GetScriptsDirectory());
    for (int i = 0; i < sizeof BUILD_SYSTEM_SCRIPTS / sizeof BUILD_SYSTEM_SCRIPTS[0]; ++i) {
        std::string fullPath = scriptsPath + "/" + BUILD_SYSTEM_SCRIPTS[i];
        ret = luaL_dofile(L, fullPath.c_str());
        if (ret != 0) {
            DebugPrint("Failed to run script: %s", BUILD_SYSTEM_SCRIPTS[i]);
            DebugPrint("Error: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
            exit(1);
        }
    }

    return L;
}

static void ResetBuildSystem(lua_State* L)
{
    lua_getglobal(L, "BuildSystem");
    lua_pushstring(L, "Reset");
    lua_gettable(L, -2);
    lua_getglobal(L, "BuildSystem");

    if (lua_pcall(L, 1, 0, 0) != 0) {
        DebugPrint("Error in Lua script.");
        DebugPrint("Error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        exit(1);
    }
}

static bool CompileOneFile(lua_State* L)
{
    lua_getglobal(L, "BuildSystem");
    lua_pushstring(L, "CompileNext");
    lua_gettable(L, -2);
    lua_getglobal(L, "BuildSystem");
    lua_pushlightuserdata(L, (void*)&KEY_RULES);
    lua_gettable(L, LUA_REGISTRYINDEX);
    lua_pushlightuserdata(L, (void*)&KEY_TASKS);
    lua_gettable(L, LUA_REGISTRYINDEX);

    if (lua_pcall(L, 3, 1, 0) != 0) {
        DebugPrint("Error in Lua script.");
        DebugPrint("Error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        exit(1);
    }

    bool result = (bool)lua_toboolean(L, -1);
    return result;
}

void AssetPipeline::CompileProc(AssetPipeline* this_)
{
    lua_State* L = NULL;

    std::string currentDir;
    std::string nextDir;

    for (;;) {
        {
            std::unique_lock<std::mutex> lock(this_->m_mutex);
            this_->m_condVar.wait(lock, [=] { return this_->m_compileInProgress; });
            nextDir = this_->m_nextDir;
        }

        if (nextDir.empty())
            break; // signals quit

        if (nextDir != currentDir) {
            currentDir = nextDir;
            AssetPipelineOsFuncs::SetWorkingDirectory(currentDir.c_str());
            if (L != NULL)
                lua_close(L);
            L = SetupLuaState(currentDir.c_str());
        } else {
            ResetBuildSystem(L);
        }

        for (;;) {
            bool compiling;
            {
                std::lock_guard<std::mutex> lock(this_->m_mutex);
                compiling = this_->m_compileInProgress;
            }
            if (!compiling)
                break;

            // Compile one file
            bool done = CompileOneFile(L);
            if (done) {
                {
                    std::lock_guard<std::mutex> lock(this_->m_mutex);
                    this_->m_compileInProgress = false;
                }
                break;
            }
        }
    }

    if (L != NULL)
        lua_close(L);
}

void AssetPipeline::Compile()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_compileInProgress = true;
    }
    m_condVar.notify_all();
}
