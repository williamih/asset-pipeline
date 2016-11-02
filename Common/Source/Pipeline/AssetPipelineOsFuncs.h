#ifndef PIPELINE_ASSETPIPELINEOSFUNCS_H
#define PIPELINE_ASSETPIPELINEOSFUNCS_H

#include <string>
#include <Core/Types.h>

namespace AssetPipelineOsFuncs {
    std::string GetPathToProjectDB();
    std::string GetScriptsDirectory();

    u64 GetTimeStamp(const char* path);

    void SetWorkingDirectory(const char* path);
}

#endif // PIPELINE_ASSETPIPELINEOSFUNCS_H
