#include "AssetPipeline.h"
#include <string>
#include <lua.hpp>
#include <Core/Macros.h>
#include "AssetPipelineOsFuncs.h"
#include "Process.h"

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
static const char KEY_CONTENTDIR = 0;
static const char KEY_MANIFEST = 0;

static int lua_Rule(lua_State* L)
{
    if (lua_gettop(L) != 2 ||
        (!lua_isstring(L, 1) && !lua_istable(L, 1)) ||
         !lua_istable(L, 2))
        return luaL_error(L, "Usage: Rule(name or { names }, "
                             "{ Parse = func, Execute = func })");

    lua_pushlightuserdata(L, (void*)&KEY_RULES);
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

static int lua_Manifest(lua_State* L)
{
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1))
        return luaL_error(L, "Usage: Manifest(path)");

    lua_pushlightuserdata(L, (void*)&KEY_MANIFEST);
    lua_pushvalue(L, 1);
    lua_settable(L, LUA_REGISTRYINDEX);

    return 0;
}

static int lua_GetManifestPath(lua_State* L)
{
    if (lua_gettop(L) != 0)
        return luaL_error(L, "Usage: GetManifestPath()");

    lua_pushlightuserdata(L, (void*)&KEY_MANIFEST);
    lua_gettable(L, LUA_REGISTRYINDEX);

    return 1;
}

static int lua_RunProcess(lua_State* L)
{
    int nArgs = lua_gettop(L);
    bool error = (nArgs == 0);
    for (int i = 1; i <= nArgs; ++i) {
        if (!lua_isstring(L, 1)) {
            error = true;
            break;
        }
    }
    if (error)
        return luaL_error(L, "Usage: RunProcess(command, arg1, arg2, arg3, ...)");

    const char* command = lua_tostring(L, 1);
    std::vector<const char*> args;
    for (int i = 1; i <= nArgs; ++i) {
        args.push_back(lua_tostring(L, i));
    }
    args.push_back(NULL);

    Process process(command, args);
    if (process.result == PROCESS_SUCCESS) {
        lua_pushinteger(L, process.status);
        lua_pushstring(L, process.stdoutStr.c_str());
        lua_pushstring(L, process.stderrStr.c_str());
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 3;
}

static lua_State* SetupLuaState(const char* projectPath)
{
    lua_State* L = luaL_newstate();

    luaL_openlibs(L);

    lua_pushlightuserdata(L, (void*)&KEY_RULES);
    lua_newtable(L);
    lua_settable(L, LUA_REGISTRYINDEX);

    lua_register(L, "Rule", lua_Rule);
    lua_register(L, "ContentDir", lua_ContentDir);
    lua_register(L, "Manifest", lua_Manifest);
    lua_register(L, "GetManifestPath", lua_GetManifestPath);
    lua_register(L, "RunProcess", lua_RunProcess);

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

    if (lua_pcall(L, 2, 1, 0) != 0) {
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
