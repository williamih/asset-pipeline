#ifndef PIPELINE_ASSETEVENTSERVICE_H
#define PIPELINE_ASSETEVENTSERVICE_H

#include <thread>
#include <queue>

#include <Core/Types.h>

class TcpSocket;

class AssetEventService {
public:
    AssetEventService();
    ~AssetEventService();

    void NotifyAssetCompiled(const char* asset);

private:
    AssetEventService(const AssetEventService&);
    AssetEventService& operator=(const AssetEventService&);

    struct Message {
        u32 size;
        void* bytes;
    };

    void ThreadProc();
    bool SendQueuedMessages(TcpSocket& socket);

    std::thread m_thread;
    std::mutex m_messageQueueMutex;
    std::queue<Message> m_messageQueue;
    std::condition_variable m_condVar;
};

#endif // PIPELINE_ASSETEVENTSERVICE_H
