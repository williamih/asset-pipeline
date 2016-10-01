#include "StrUtils.h"

// TODO: Does this work with paths that have . or .. components?
std::string StrUtilsMakeRelativePath(const char* basePath, const char* path)
{
    while (*basePath != '\0' && *basePath == *path) {
        ++basePath;
        ++path;
    }
    if (*basePath == '\0') {
        if (*path == '/' || *path == '\\')
            ++path;
        return path;
    }
    return std::string();
}
