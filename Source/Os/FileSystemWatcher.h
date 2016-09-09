#ifndef OS_FILESYSTEMWATCHER_H
#define OS_FILESYSTEMWATCHER_H

#include <functional>

class FileSystemWatcher {
public:
    enum EventType {
        FILE_CREATED,
        FILE_REMOVED,
        FILE_RENAMED,
        FILE_MODIFIED,
    };

    typedef std::function<void(EventType, const char*)> OnFileChangedFunc;

    static FileSystemWatcher* Create();
    static void Destroy(FileSystemWatcher* watcher);

    // N.B. The callback specified will always be called on the main thread!
    void SetOnFileChanged(const OnFileChangedFunc& func);

    void WatchDirectory(const char* path);

private:
    FileSystemWatcher();
    ~FileSystemWatcher();
    FileSystemWatcher(const FileSystemWatcher&);
    FileSystemWatcher& operator=(const FileSystemWatcher&);
};

#endif // OS_FILESYSTEMWATCHER_H
