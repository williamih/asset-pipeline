#include "AssetPipelineOsFuncs.h"
#import <Cocoa/Cocoa.h>
#include <sys/stat.h>
#include <utime.h>
#include <Core/Macros.h>

std::string AssetPipelineOsFuncs::GetPathToProjectDB()
{
    NSArray* array = NSSearchPathForDirectoriesInDomains(
        NSApplicationSupportDirectory,
        NSUserDomainMask,
        YES // expandTilde
    );
    NSString* dir = [[array objectAtIndex:0] stringByAppendingPathComponent:@"Asset Pipeline"];
    [[NSFileManager defaultManager] createDirectoryAtPath:dir
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:nil];
    NSString* path = [dir stringByAppendingPathComponent:@"ProjectDB.sqlite3"];
    return [path UTF8String];
}

std::string AssetPipelineOsFuncs::GetScriptsDirectory()
{
    return [[[NSBundle mainBundle] resourcePath] UTF8String];
}

u64 AssetPipelineOsFuncs::GetTimeStamp(const char* path)
{
    struct stat st;
    if (stat(path, &st) == -1)
        FATAL("stat");
    // Millisecond accuracy.
    u64 result = 0;
    result += (u64)st.st_mtimespec.tv_sec * 1000;
    result += (u64)st.st_mtimespec.tv_nsec / 1000000;
    return result;
}

void AssetPipelineOsFuncs::SetWorkingDirectory(const char* path)
{
    chdir(path);
}
