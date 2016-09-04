#ifdef __APPLE__

#include "FileSystemWatcher.h"
#include <stdio.h>
#include <CoreServices/CoreServices.h>

#include <Core/Macros.h>

class FileSystemWatcherMac {
public:
    FileSystemWatcherMac();
    ~FileSystemWatcherMac();

    void SetOnFileChanged(const FileSystemWatcher::OnFileChangedFunc& func);
    void WatchDirectory(const char* path);

    void FileChangedInternal(FileSystemWatcher::EventType event, const char* path);

private:
    FileSystemWatcherMac(const FileSystemWatcherMac&);
    FileSystemWatcherMac& operator=(const FileSystemWatcherMac&);

    FSEventStreamRef m_eventStream;
    FileSystemWatcher::OnFileChangedFunc m_onFileChanged;
};

FileSystemWatcherMac::FileSystemWatcherMac()
    : m_eventStream(NULL)
{}

FileSystemWatcherMac::~FileSystemWatcherMac()
{
    if (m_eventStream)
        FSEventStreamRelease((FSEventStreamRef)m_eventStream);
}

void FileSystemWatcherMac::SetOnFileChanged(const FileSystemWatcher::OnFileChangedFunc& func)
{
    m_onFileChanged = func;
}

static void EventCallback(ConstFSEventStreamRef streamRef,
                          void *clientCallBackInfo,
                          size_t numEvents,
                          void *eventPaths,
                          const FSEventStreamEventFlags eventFlags[],
                          const FSEventStreamEventId eventIds[])
{
    FileSystemWatcherMac* watcher = (FileSystemWatcherMac*)clientCallBackInfo;
    for (size_t i = 0; i < numEvents; ++i) {
        const char* path = ((const char**)eventPaths)[i];
        if (eventFlags[i] & kFSEventStreamEventFlagMustScanSubDirs) {
            DebugPrint("FileSystemWatcher: an error occurred.\n");
            continue;
        }
        if (eventFlags[i] & kFSEventStreamEventFlagItemCreated) {
            watcher->FileChangedInternal(FileSystemWatcher::FILE_CREATED, path);
        }
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRemoved) {
            watcher->FileChangedInternal(FileSystemWatcher::FILE_REMOVED, path);
        }
        else if (eventFlags[i] & kFSEventStreamEventFlagItemRenamed) {
            watcher->FileChangedInternal(FileSystemWatcher::FILE_RENAMED, path);
        }
        else if (eventFlags[i] & kFSEventStreamEventFlagItemModified) {
            watcher->FileChangedInternal(FileSystemWatcher::FILE_MODIFIED, path);
        }
    }
}

void FileSystemWatcherMac::WatchDirectory(const char* path)
{
    static const CFTimeInterval LATENCY_IN_SECONDS = 0.25;

    if (!CFRunLoopGetCurrent())
        FATAL("Must have a run loop active before calling WatchDirectory().");

    if (m_eventStream) {
        FSEventStreamRelease(m_eventStream);
        m_eventStream = NULL;
    }

    FSEventStreamContext context;
    context.version = 0;
    context.info = (void*)this;
    context.retain = NULL;
    context.release = NULL;
    context.copyDescription = NULL;
    CFStringRef dir = CFStringCreateWithFileSystemRepresentation(NULL, path);
    CFArrayRef pathsToWatch = CFArrayCreate(
        NULL,
        (const void**)&dir,
        1,
        &kCFTypeArrayCallBacks
    );
    FSEventStreamRef eventStream = FSEventStreamCreate(
        NULL,
        EventCallback,
        &context,
        pathsToWatch,
        kFSEventStreamEventIdSinceNow,
        LATENCY_IN_SECONDS,
        kFSEventStreamCreateFlagFileEvents
    );

    FSEventStreamScheduleWithRunLoop(eventStream, CFRunLoopGetCurrent(),
                                     kCFRunLoopDefaultMode);
    FSEventStreamStart(eventStream);

    m_eventStream = eventStream;
}

void FileSystemWatcherMac::FileChangedInternal(FileSystemWatcher::EventType event,
                                               const char* path)
{
    if (m_onFileChanged)
        m_onFileChanged(event, path);
}

static FileSystemWatcherMac* Cast(FileSystemWatcher* watcher)
{
    return (FileSystemWatcherMac*)watcher;
}

static FileSystemWatcher* Cast(FileSystemWatcherMac* watcher)
{
    return (FileSystemWatcher*)watcher;
}

FileSystemWatcher* FileSystemWatcher::Create()
{ return Cast(new FileSystemWatcherMac); }

void FileSystemWatcher::Destroy(FileSystemWatcher* watcher)
{ delete Cast(watcher); }

void FileSystemWatcher::SetOnFileChanged(const OnFileChangedFunc& func)
{ Cast(this)->SetOnFileChanged(func); }

void FileSystemWatcher::WatchDirectory(const char* path)
{ Cast(this)->WatchDirectory(path); }

#endif // __APPLE__
