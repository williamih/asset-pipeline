#ifndef PIPELINE_ASSETPIPELINE_H
#define PIPELINE_ASSETPIPELINE_H

#include <functional>
#include <thread>
#include <string>
#include <queue>

struct lua_State;

class AssetPipelineDelegate {
public:
    virtual ~AssetPipelineDelegate() {}

    virtual void OnAssetBuildFinished(int nCompiledSuccessfully) {}
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

    AssetPipeline(const AssetPipeline&);
    AssetPipeline& operator=(const AssetPipeline&);

    void PushMessage(const MsgFunc& message);
    static void CompileProc(AssetPipeline* this_);

    std::thread m_thread;
    std::string m_nextDir;
    std::mutex m_mutex;
    bool m_compileInProgress;
    std::condition_variable m_condVar;

    AssetPipelineDelegate* m_delegate;

    std::queue<MsgFunc> m_messageQueue;
    std::mutex m_messageQueueMutex;
};

#endif // PIPELINE_ASSETPIPELINE_H
