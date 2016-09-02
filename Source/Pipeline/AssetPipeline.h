#ifndef PIPELINE_ASSETPIPELINE_H
#define PIPELINE_ASSETPIPELINE_H

#include <functional>
#include <thread>
#include <string>
#include <vector>
#include <queue>
#include "AssetEventService.h"

struct lua_State;

struct AssetCompileFailureInfo {
    std::vector<std::string> inputPaths;
    std::vector<std::string> outputPaths;
    std::string errorMessage;
};

class AssetPipelineDelegate {
public:
    virtual ~AssetPipelineDelegate() {}

    virtual void OnAssetBuildFinished(int nSucceeded, int nFailed) {}
    virtual void OnAssetFailedToCompile(const AssetCompileFailureInfo& info) {}
};

class AssetPipeline {
public:
    AssetPipeline();
    ~AssetPipeline();

    void SetProjectWithDirectory(const char* path);
    void Compile();

    AssetPipelineDelegate* GetDelegate() const;
    void SetDelegate(AssetPipelineDelegate* delegate);

    void CallDelegateFunctions();

private:
    typedef std::function<void(AssetPipelineDelegate*)> MsgFunc;
public: // NOT for use by user code
    void PushMessage(const MsgFunc& message);
private:

    AssetPipeline(const AssetPipeline&);
    AssetPipeline& operator=(const AssetPipeline&);

    static void lua_RecordCompileError(lua_State* L);
    static void CompileProc(AssetPipeline* this_);

    std::thread m_thread;
    std::string m_nextDir;
    std::mutex m_mutex;
    bool m_compileInProgress;
    std::condition_variable m_condVar;

    AssetPipelineDelegate* m_delegate;

    std::queue<MsgFunc> m_messageQueue;
    std::mutex m_messageQueueMutex;

    AssetEventService m_assetEventService;
};

#endif // PIPELINE_ASSETPIPELINE_H
