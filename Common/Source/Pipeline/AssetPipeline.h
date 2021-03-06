#ifndef PIPELINE_ASSETPIPELINE_H
#define PIPELINE_ASSETPIPELINE_H

#include <functional>
#include <thread>
#include <string>
#include <vector>
#include <queue>
#include <Os/FileSystemWatcher.h>
#include "AssetEventService.h"

class ProjectDBConn;

struct AssetBuildCompletionInfo {
    int projectID;
    int nSucceeded;
    int nFailed;
};

struct AssetRecompileInfo {
    int projectID;
    std::string path;
    bool succeeded;
};

struct AssetCompileFailureInfo {
    std::vector<std::string> inputPaths;
    std::vector<std::string> additionalInputPaths;
    std::vector<std::string> outputPaths;
    std::string errorMessage;
};

class AssetPipelineDelegate {
public:
    enum BuildType {
        BUILD_SINGLE_FILE,
        BUILD_PROJECT,
    };

    virtual ~AssetPipelineDelegate() {}

    virtual void OnAssetBuildFinished(const AssetBuildCompletionInfo& info) = 0;
    virtual void OnAssetRecompileFinished(const AssetRecompileInfo& info) = 0;
    virtual void OnAssetCompileSucceeded() = 0;
    virtual void OnAssetFailedToCompile(const AssetCompileFailureInfo& info) = 0;
};

class AssetPipeline {
public:
    explicit AssetPipeline();
    ~AssetPipeline();

    void CompileProject(int projectID);

    AssetPipelineDelegate* GetDelegate() const;
    void SetDelegate(AssetPipelineDelegate* delegate);

    void CallDelegateFunctions();

private:
    typedef std::function<void(AssetPipelineDelegate*)> MsgFunc;
public: // NOT for use by user code
    void PushMessage(const MsgFunc& message);
private:

    struct CompileQueueItem {
        // If projectID is negative, this item represents a single modified
        // file, specified by modifiedFilePath.
        // Otherwise, this represents compilation of a project, and
        // modifiedFilePath is ignored.
        int projectID;
        std::string modifiedFilePath;
    };

    AssetPipeline(const AssetPipeline&);
    AssetPipeline& operator=(const AssetPipeline&);

    void PushCompileQueueItem(const CompileQueueItem& item);
    void FileSystemWatcherCallback(FileSystemWatcher::EventType event, const char* path);
    static void CompileProc(AssetPipeline* this_);

    std::thread m_thread;
    std::queue<CompileQueueItem> m_compileQueue;
    std::mutex m_mutex;
    bool m_compileInProgress;
    std::condition_variable m_condVar;

    AssetPipelineDelegate* m_delegate;

    std::queue<MsgFunc> m_messageQueue;
    std::mutex m_messageQueueMutex;

    AssetEventService m_assetEventService;
};

#endif // PIPELINE_ASSETPIPELINE_H
