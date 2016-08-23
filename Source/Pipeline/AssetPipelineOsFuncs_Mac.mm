#include "AssetPipelineOsFuncs.h"
#import <Cocoa/Cocoa.h>

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
