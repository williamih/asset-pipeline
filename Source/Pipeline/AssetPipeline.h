#ifndef PIPELINE_ASSETPIPELINE_H
#define PIPELINE_ASSETPIPELINE_H

#include <thread>
#include <string>

struct lua_State;

class AssetPipeline {
public:
    AssetPipeline();
    ~AssetPipeline();

    void SetProjectWithDirectory(const char* path);
    void Compile();

private:
    AssetPipeline(const AssetPipeline&);
    AssetPipeline& operator=(const AssetPipeline&);

    static void CompileProc(AssetPipeline* this_);

    std::thread m_thread;
    std::string m_nextDir;
    std::mutex m_mutex;
    bool m_compileInProgress;
    std::condition_variable m_condVar;
};

#endif // PIPELINE_ASSETPIPELINE_H
