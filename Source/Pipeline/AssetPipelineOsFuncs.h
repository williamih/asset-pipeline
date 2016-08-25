#ifndef PIPELINE_ASSETPIPELINEOSFUNCS_H
#define PIPELINE_ASSETPIPELINEOSFUNCS_H

#include <string>
#include <functional>

namespace AssetPipelineOsFuncs {
    std::string GetPathToProjectDB();
    std::string GetScriptsDirectory();

    void SetWorkingDirectory(const char* path);

    class DirectoryIterator {
    public:
        static DirectoryIterator* Create(const char* path);
        static void Destroy(DirectoryIterator* iter);

        std::pair<std::string, bool> Next();

    private:
        DirectoryIterator();
        ~DirectoryIterator();
    };
}

#endif // PIPELINE_ASSETPIPELINEOSFUNCS_H
