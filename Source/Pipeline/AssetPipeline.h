#ifndef PIPELINE_ASSETPIPELINE_H
#define PIPELINE_ASSETPIPELINE_H

#include <thread>
#include <string>
#include <queue>

struct lua_State;

enum AssetPipelineMessageType {
    ASSET_PIPELINE_COMPILE_FINISHED,
};

struct AssetPipelineMessage {
    AssetPipelineMessageType type;
    int intValue;
};

class AssetPipeline {
public:
    AssetPipeline();
    ~AssetPipeline();

    void SetProjectWithDirectory(const char* path);
    void Compile();

    bool PollMessage(AssetPipelineMessage* message);

private:
    AssetPipeline(const AssetPipeline&);
    AssetPipeline& operator=(const AssetPipeline&);

    void PushMessage(const AssetPipelineMessage& message);
    static void CompileProc(AssetPipeline* this_);

    std::thread m_thread;
    std::string m_nextDir;
    std::mutex m_mutex;
    bool m_compileInProgress;
    std::condition_variable m_condVar;

    std::queue<AssetPipelineMessage> m_messageQueue;
    std::mutex m_messageQueueMutex;
};

#endif // PIPELINE_ASSETPIPELINE_H
