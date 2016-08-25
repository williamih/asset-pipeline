#include "AssetPipelineOsFuncs.h"
#import <Cocoa/Cocoa.h>

namespace AssetPipelineOsFuncs {
    class DirectoryIteratorMac {
    public:
        explicit DirectoryIteratorMac(const char* path);

        std::pair<std::string, bool> Next();

    private:
        NSDirectoryEnumerator* m_enumerator;
    };
}

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

void AssetPipelineOsFuncs::SetWorkingDirectory(const char* path)
{
    chdir(path);
}

AssetPipelineOsFuncs::DirectoryIteratorMac::DirectoryIteratorMac(const char* path)
    : m_enumerator(NULL)
{
    NSString* nsPath = [NSString stringWithUTF8String:path];
    m_enumerator = [[NSFileManager defaultManager] enumeratorAtPath:nsPath];
}

std::pair<std::string, bool> AssetPipelineOsFuncs::DirectoryIteratorMac::Next()
{
    NSString* str = [m_enumerator nextObject];
    if (str != nil)
        return std::make_pair([str UTF8String], true);
    return std::make_pair("", false);
}

AssetPipelineOsFuncs::DirectoryIterator*
AssetPipelineOsFuncs::DirectoryIterator::Create(const char* path)
{
    return (DirectoryIterator*)new DirectoryIteratorMac(path);
}

void AssetPipelineOsFuncs::DirectoryIterator::Destroy(DirectoryIterator* iter)
{
    delete (DirectoryIteratorMac*)iter;
}

std::pair<std::string, bool> AssetPipelineOsFuncs::DirectoryIterator::Next()
{
    return ((DirectoryIteratorMac*)this)->Next();
}
