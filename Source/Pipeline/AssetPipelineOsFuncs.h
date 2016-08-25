#ifndef PIPELINE_ASSETPIPELINEOSFUNCS_H
#define PIPELINE_ASSETPIPELINEOSFUNCS_H

#include <vector>
#include <string>
#include <functional>

namespace AssetPipelineOsFuncs {
    std::string GetPathToProjectDB();
    std::string GetScriptsDirectory();

    void SetWorkingDirectory(const char* path);

    enum ProcessCreationResult {
        PROCESS_SUCCESS,
        PROCESS_NOT_FOUND
    };

    struct Process {
        Process(const char* path, const std::vector<const char*>& args);

        ProcessCreationResult result;
        int status;
        std::string stdoutStr;
        std::string stderrStr;
    };
}

#endif // PIPELINE_ASSETPIPELINEOSFUNCS_H
