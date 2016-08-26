#ifndef PIPELINE_ASSETPIPELINEOSFUNCS_H
#define PIPELINE_ASSETPIPELINEOSFUNCS_H

#include <string>

namespace AssetPipelineOsFuncs {
    std::string GetPathToProjectDB();
    std::string GetScriptsDirectory();

    void SetWorkingDirectory(const char* path);
}

#endif // PIPELINE_ASSETPIPELINEOSFUNCS_H
