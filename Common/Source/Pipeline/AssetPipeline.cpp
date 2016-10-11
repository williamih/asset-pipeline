#include "AssetPipeline.h"

#include <string>
#include <memory>
#include <limits.h>
#include <lua.hpp>

#include <Core/Macros.h>
#include <Os/FileSystemWatcher.h>

#include "AssetPipelineOsFuncs.h"
#include "Process.h"
#include "StrUtils.h"
#include "ProjectDBConn.h"

const char* const BUILD_SCRIPT_RELATIVE_PATH = "assetpipeline.lua";

static const char* const BUILD_SYSTEM_SCRIPTS[] = {
    "list.lua",
    "buildsystem.lua",
};

AssetPipeline::AssetPipeline()
    : m_thread(&AssetPipeline::CompileProc, this)
    , m_compileQueue()
    , m_mutex()
    , m_compileInProgress(false)
    , m_condVar()

    , m_delegate(NULL)

    , m_messageQueue()
    , m_messageQueueMutex()

    , m_assetEventService()
{}

AssetPipeline::~AssetPipeline()
{
    // Set up special conditions that let the thread know it's time to exit.
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_compileQueue.empty())
            m_compileQueue.pop();
        m_compileInProgress = true;
    }
    m_condVar.notify_all();

    m_thread.join();
}

void AssetPipeline::CompileProject(int projectID)
{
    ASSERT(projectID >= 0);
    CompileQueueItem item;
    item.projectID = projectID;

    PushCompileQueueItem(item);
}

void AssetPipeline::CallDelegateFunctions()
{
    if (!m_delegate)
        return;
    MsgFunc func;
    for (;;) {
        {
            std::lock_guard<std::mutex> lock(m_messageQueueMutex);
            if (m_messageQueue.empty())
                break;
            func = m_messageQueue.front();
            m_messageQueue.pop();
        }
        func(m_delegate);
    }
}

void AssetPipeline::PushMessage(const MsgFunc& message)
{
    std::lock_guard<std::mutex> lock(m_messageQueueMutex);
    m_messageQueue.push(message);
}

void AssetPipeline::PushCompileQueueItem(const CompileQueueItem& item)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_compileQueue.push(item);
        m_compileInProgress = true;
    }
    m_condVar.notify_all();
}

// TODO: Refactor code e.g. by creating a function to add/retrieve values from
// the Lua registry.

static const char KEY_RULES = 0;
static const char KEY_CONTENTDIR = 0;
static const char KEY_DATADIR = 0;
static const char KEY_MANIFEST = 0;
static const char KEY_THIS = 0;
static const char KEY_ASSETEVENTSERVICE = 0;
static const char KEY_PROJECTDBCONN = 0;
static const char KEY_PROJECTID = 0;

namespace {
    template<class T>
    struct RegistryAccessor;

    template<class U>
    struct RegistryAccessor<U*> {
        static U* Get(lua_State* L, const char* key)
        {
            lua_pushlightuserdata(L, (void*)key);
            lua_gettable(L, LUA_REGISTRYINDEX);
            void* ptr = lua_touserdata(L, -1);
            lua_pop(L, 1);
            return (U*)ptr;
        }

        static void Set(lua_State* L, const char* key, U* value)
        {
            lua_pushlightuserdata(L, (void*)key);
            lua_pushlightuserdata(L, (void*)value);
            lua_settable(L, LUA_REGISTRYINDEX);
        }
    };

    template<>
    struct RegistryAccessor<int> {
        static int Get(lua_State* L, const char* key)
        {
            lua_pushlightuserdata(L, (void*)key);
            lua_gettable(L, LUA_REGISTRYINDEX);
            int n = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            return n;
        }

        static void Set(lua_State* L, const char* key, int value)
        {
            lua_pushlightuserdata(L, (void*)key);
            lua_pushinteger(L, (lua_Integer)value);
            lua_settable(L, LUA_REGISTRYINDEX);
        }
    };

    template<class T>
    T GetFromRegistry(lua_State* L, const char* key)
    {
        return RegistryAccessor<T>::Get(L, key);
    }

    template<class T>
    void SetInRegistry(lua_State* L, const char* key, T value)
    {
        RegistryAccessor<T>::Set(L, key, value);
    }
}

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

static int lua_DataDir(lua_State* L)
{
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1))
        return luaL_error(L, "Usage: DataDir(path)");

    lua_pushlightuserdata(L, (void*)&KEY_DATADIR);
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

static void StringTableToVector(lua_State* L, int tableIndex,
                                std::vector<std::string>* vec)
{
    ASSERT(vec);
    // this method doesn't support negative stack indices
    ASSERT(tableIndex >= 0);
    int size = luaL_getn(L, tableIndex);
    for (int i = 1; i <= size; ++i) {
        lua_pushinteger(L, i);
        lua_gettable(L, tableIndex);
        if (!lua_isstring(L, -1))
            luaL_error(L, "Expected string, got %s", luaL_typename(L, -1));
        const char* str = lua_tostring(L, -1);
        vec->push_back(str);
    }
}

static int lua_RecordCompileError(lua_State* L)
{
    if (lua_gettop(L) != 3 || !lua_istable(L, 1) || !lua_istable(L, 2) ||
        !lua_isstring(L, 3))
        return luaL_error(
            L, "Usage: RecordCompileError(inputsTable, outputsTable, errorMsg)"
        );

    AssetCompileFailureInfo info;
    StringTableToVector(L, 1, &info.inputPaths);
    StringTableToVector(L, 2, &info.outputPaths);
    info.errorMessage = lua_tostring(L, 3);

    AssetPipeline* this_ = GetFromRegistry<AssetPipeline*>(L, &KEY_THIS);

    this_->PushMessage(std::bind(
        &AssetPipelineDelegate::OnAssetFailedToCompile,
        std::placeholders::_1,
        info
    ));

    ProjectDBConn* conn = GetFromRegistry<ProjectDBConn*>(L, &KEY_PROJECTDBCONN);
    int projID = GetFromRegistry<int>(L, &KEY_PROJECTID);

    conn->RecordError(projID, info.inputPaths, info.outputPaths, info.errorMessage);

    return 0;
}

static int lua_ClearCompileError(lua_State* L)
{
    if (lua_gettop(L) != 2 || !lua_istable(L, 1) || !lua_istable(L, 2))
        return luaL_error(
            L, "Usage: ClearCompileError(inputsTable, outputsTable)"
        );

    std::vector<std::string> inputPaths;
    std::vector<std::string> outputPaths;
    StringTableToVector(L, 1, &inputPaths);
    StringTableToVector(L, 2, &outputPaths);

    ProjectDBConn* conn = GetFromRegistry<ProjectDBConn*>(L, &KEY_PROJECTDBCONN);
    int projID = GetFromRegistry<int>(L, &KEY_PROJECTID);

    conn->ClearError(projID, inputPaths, outputPaths);

    return 0;
}

static int lua_NotifyAssetCompile(lua_State* L)
{
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1))
        return luaL_error(L, "Usage: NotifyAssetCompile(\"path/to/asset\"");

    const char* path = lua_tostring(L, 1);

    AssetEventService* service = GetFromRegistry<AssetEventService*>(L, &KEY_ASSETEVENTSERVICE);

    lua_pushlightuserdata(L, (void*)&KEY_DATADIR);
    lua_gettable(L, LUA_REGISTRYINDEX);
    const char* dataDirectory = NULL;
    if (!lua_isnil(L, -1))
        dataDirectory = lua_tostring(L, -1);
    lua_pop(L, 1);

    if (dataDirectory != NULL) {
        std::string relativePath = StrUtilsMakeRelativePath(dataDirectory, path);
        // Convert slashes to backslashes
        for (size_t i = 0; i < relativePath.size(); ++i) {
            if (relativePath[i] == '/')
                relativePath[i] = '\\';
        }

        if (!relativePath.empty())
            service->NotifyAssetCompiled(relativePath.c_str());
    }

    return 0;
}

static int lua_ClearDependencies(lua_State* L)
{
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1))
        return luaL_error(L, "Usage: ClearDependencies(\"path/to/asset\"");

    const char* outputPath = lua_tostring(L, 1);

    ProjectDBConn* conn = GetFromRegistry<ProjectDBConn*>(L, &KEY_PROJECTDBCONN);

    int projIdx = GetFromRegistry<int>(L, &KEY_PROJECTID);

    conn->ClearDependencies(projIdx, outputPath);

    return 0;
}

static int lua_RecordDependency(lua_State* L)
{
    if (lua_gettop(L) != 2 || !lua_isstring(L, 1) || !lua_isstring(L, 2))
        return luaL_error(L, "Usage: RecordDependency(\"outputPath\", \"inputPath\"");

    const char* outputPath = lua_tostring(L, 1);
    const char* inputPath = lua_tostring(L, 2);

    ProjectDBConn* conn = GetFromRegistry<ProjectDBConn*>(L, &KEY_PROJECTDBCONN);

    int projIdx = GetFromRegistry<int>(L, &KEY_PROJECTID);

    conn->RecordDependency(projIdx, outputPath, inputPath);

    return 0;
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

static int lua_GetFileTimestamp(lua_State* L)
{
    if (lua_gettop(L) != 1 || !lua_isstring(L, 1))
        return luaL_error(L, "Usage: GetFileTimestamp(\"path/to/file\"");
    const char* path = lua_tostring(L, -1);
    lua_pushnumber(L, (lua_Number)AssetPipelineOsFuncs::GetTimeStamp(path));
    return 1;
}

static lua_State* SetupLuaState(int projectID,
                                const char* projectPath,
                                AssetPipeline* pipeline,
                                AssetEventService* assetEventService,
                                ProjectDBConn* projectDBConn)
{
    ASSERT(projectPath);
    ASSERT(pipeline);
    ASSERT(assetEventService);

    lua_State* L = luaL_newstate();

    luaL_openlibs(L);

    lua_pushlightuserdata(L, (void*)&KEY_RULES);
    lua_newtable(L);
    lua_settable(L, LUA_REGISTRYINDEX);

    SetInRegistry(L, &KEY_THIS, pipeline);
    SetInRegistry(L, &KEY_ASSETEVENTSERVICE, assetEventService);
    SetInRegistry(L, &KEY_PROJECTDBCONN, projectDBConn);
    SetInRegistry(L, &KEY_PROJECTID, projectID);

    lua_register(L, "Rule", lua_Rule);
    lua_register(L, "ContentDir", lua_ContentDir);
    lua_register(L, "DataDir", lua_DataDir);
    lua_register(L, "Manifest", lua_Manifest);
    lua_register(L, "GetManifestPath", lua_GetManifestPath);
    lua_register(L, "RecordCompileError", lua_RecordCompileError);
    lua_register(L, "ClearCompileError", lua_ClearCompileError);
    lua_register(L, "RunProcess", lua_RunProcess);
    lua_register(L, "GetFileTimestamp", lua_GetFileTimestamp);
    lua_register(L, "NotifyAssetCompile", lua_NotifyAssetCompile);
    lua_register(L, "ClearDependencies", lua_ClearDependencies);
    lua_register(L, "RecordDependency", lua_RecordDependency);

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

// N.B. paths may be NULL
static void SetupBuildSystem(lua_State* L, std::vector<std::string>* paths)
{
    lua_getglobal(L, "BuildSystem");
    lua_pushstring(L, "Setup");
    lua_gettable(L, -2);

    lua_getglobal(L, "BuildSystem");

    if (paths) {
        lua_newtable(L);
        for (size_t i = 0; i < paths->size(); ++i) {
            lua_pushstring(L, (*paths)[i].c_str());
            int key = (int)i + 1;
            lua_rawseti(L, -2, key);
        }
    } else {
        lua_pushnil(L);
    }

    if (lua_pcall(L, 2, 0, 0) != 0) {
        DebugPrint("Error in Lua script.");
        DebugPrint("Error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        exit(1);
    }
}

static void CompileOneFile(lua_State* L,
                           bool* hadRemainingAsset,
                           bool* succeeded)
{
    ASSERT(hadRemainingAsset);
    ASSERT(succeeded);

    lua_getglobal(L, "BuildSystem");
    lua_pushstring(L, "CompileNext");
    lua_gettable(L, -2);
    lua_getglobal(L, "BuildSystem");
    lua_pushlightuserdata(L, (void*)&KEY_RULES);
    lua_gettable(L, LUA_REGISTRYINDEX);

    if (lua_pcall(L, 2, 2, 0) != 0) {
        DebugPrint("Error in Lua script.");
        DebugPrint("Error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
        exit(1);
    }

    *hadRemainingAsset = (bool)lua_toboolean(L, -2);
    *succeeded = (bool)lua_toboolean(L, -1);
}

// Returns an empty string if not found
static std::string GetContentDir(lua_State* L)
{
    lua_pushlightuserdata(L, (void*)&KEY_CONTENTDIR);
    lua_gettable(L, LUA_REGISTRYINDEX);
    std::string str;
    if (lua_isstring(L, -1))
        str = lua_tostring(L, -1);
    lua_pop(L, 1);
    return str;
}

static std::string JoinPaths(const std::string& a, const std::string& b)
{
    std::string ret;
    ret.reserve(a.length() + b.length() + 1);
    ret.append(a);
    if (a.back() != '/' && a.back() != '\\')
        ret.push_back('/');
    ret.append(b);
    return ret;
}

void AssetPipeline::FileSystemWatcherCallback(FileSystemWatcher::EventType event,
                                              const char* path)
{
    CompileQueueItem item;
    item.projectID = -1;
    item.modifiedFilePath = path;
    PushCompileQueueItem(item);
}

void AssetPipeline::CompileProc(AssetPipeline* this_)
{
    lua_State* L = NULL;

    ProjectDBConn dbConn;

    typedef std::unique_ptr<FileSystemWatcher, void (*)(FileSystemWatcher*)> FSWatcherPtr;
    FSWatcherPtr fsWatcher(FileSystemWatcher::Create(), &FileSystemWatcher::Destroy);
    fsWatcher->SetOnFileChanged([=] (FileSystemWatcher::EventType event,
                                     const char* path) {
        // WARNING: This is called on the main thread!!
        this_->FileSystemWatcherCallback(event, path);
    });

    std::string currDir;
    int currProjID = -1;

    for (;;) {
        CompileQueueItem nextItem;
        {
            std::unique_lock<std::mutex> lock(this_->m_mutex);
            this_->m_condVar.wait(lock, [=] { return this_->m_compileInProgress; });
            if (this_->m_compileQueue.empty()) {
                // Signals quit
                break;
            } else {
                nextItem = this_->m_compileQueue.front();
                this_->m_compileQueue.pop();
            }
        }

        std::string singleFilePath;
        bool recompilingSingleFile = false;

        if (nextItem.projectID < 0) {
            // We are recompiling a modified file.
            recompilingSingleFile = true;

            ASSERT(currProjID >= 0);
            ASSERT(!currDir.empty());

            std::string input = StrUtilsMakeRelativePath(
                currDir.c_str(), nextItem.modifiedFilePath.c_str()
            );
            singleFilePath = input;

            std::vector<std::string> outputs;
            dbConn.GetDependents(currProjID, input.c_str(), &outputs);

            SetupBuildSystem(L, &outputs);
        } else {
            // We are compiling a whole project.
            std::string projectDir = dbConn.GetProjectDirectory(nextItem.projectID);

            // If we're moving to a new project at a different directory, we
            // need to recreate the Lua state using a different configuration
            // script.
            if (nextItem.projectID != currProjID || projectDir != currDir) {

                currDir = projectDir;
                currProjID = nextItem.projectID;

                AssetPipelineOsFuncs::SetWorkingDirectory(projectDir.c_str());

                if (L != NULL)
                    lua_close(L);
                L = SetupLuaState(
                    nextItem.projectID,
                    projectDir.c_str(),
                    this_,
                    &this_->m_assetEventService,
                    &dbConn
                );

                std::string contentDir = GetContentDir(L);
                if (!contentDir.empty()) {
                    std::string fullPath = JoinPaths(projectDir, contentDir);
                    fsWatcher->WatchDirectory(fullPath.c_str());
                }
            }

            SetupBuildSystem(L, NULL);
        }

        int nSucceeded = 0;
        int nFailed = 0;

        for (;;) {
            bool compiling;
            {
                std::lock_guard<std::mutex> lock(this_->m_mutex);
                compiling = this_->m_compileInProgress;
            }
            if (!compiling)
                break;

            // Compile one file
            bool hadRemainingAsset;
            bool succeeded;
            CompileOneFile(L, &hadRemainingAsset, &succeeded);

            if (!hadRemainingAsset) {
                // Compilation process is done
                {
                    std::lock_guard<std::mutex> lock(this_->m_mutex);
                    this_->m_compileInProgress = false;
                }
                ASSERT(currProjID != -1);
                if (recompilingSingleFile) {
                    AssetRecompileInfo info;
                    info.projectID = currProjID;
                    info.path = singleFilePath;
                    info.succeeded = (nSucceeded > 0);
                    this_->PushMessage(std::bind(
                        &AssetPipelineDelegate::OnAssetRecompileFinished,
                        std::placeholders::_1,
                        info
                    ));
                } else {
                    AssetBuildCompletionInfo info;
                    info.projectID = currProjID;
                    info.nSucceeded = nSucceeded;
                    info.nFailed = nFailed;
                    this_->PushMessage(std::bind(
                        &AssetPipelineDelegate::OnAssetBuildFinished,
                        std::placeholders::_1,
                        info
                    ));
                }
                break;
            }

            if (succeeded) {
                ++nSucceeded;
                this_->PushMessage(
                    &AssetPipelineDelegate::OnAssetCompileSucceeded
                );
            } else {
                ++nFailed;
            }
        }
    }

    if (L != NULL)
        lua_close(L);
}

AssetPipelineDelegate* AssetPipeline::GetDelegate() const
{
    return m_delegate;
}

void AssetPipeline::SetDelegate(AssetPipelineDelegate* delegate)
{
    m_delegate = delegate;
}
