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

long AssetPipelineOsFuncs::GetTimeStamp(const char* path)
{
    static_assert(sizeof(long) == sizeof(time_t),
                  "Expected sizeof(long) == sizeof(time_t)");

    struct stat st;
    if (stat(path, &st) == -1)
        FATAL("stat");
    return (long)st.st_atime;
}

void AssetPipelineOsFuncs::SetTimeStamp(const char* path, long timeStamp)
{
    struct utimbuf t;
    t.actime = (time_t)timeStamp;
    t.modtime = (time_t)timeStamp;
    if (utime(path, &t) == -1)
        FATAL("utime");
}

void AssetPipelineOsFuncs::SetWorkingDirectory(const char* path)
{
    chdir(path);
}
