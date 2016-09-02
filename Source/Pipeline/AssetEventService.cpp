#include "AssetEventService.h"

#include <stdlib.h>
#include <string.h>

#include <Core/Endian.h>
#include <Core/Macros.h>
#include <Os/TcpSocket.h>

// TODO: Properly clean up the message queue and the thread

static u32 Address(u32 a, u32 b, u32 c, u32 d)
{
    return (a << 24) | (b << 16) | (c << 8) | d;
}

const u32 MSG_ASSET_COMPILED = 1;

const u32 PORT = 6789;
const u32 LOCALHOST = Address(127, 0, 0, 1);

AssetEventService::AssetEventService()
    : m_thread()
    , m_messageQueueMutex()
    , m_messageQueue()
    , m_condVar()
{
    m_thread = std::thread(&AssetEventService::ThreadProc, this);
}

AssetEventService::~AssetEventService()
{
    m_thread.join();
}

static u8* Write(u8* ptr, u32 value)
{
    value = EndianSwapLE32(value);
    memcpy(ptr, &value, sizeof value);
    return ptr + sizeof value;
}

static u8* Write(u8* ptr, const char* str, u32 len)
{
    memcpy(ptr, str, len + 1);
    return ptr + len + 1;
}

void AssetEventService::NotifyAssetCompiled(const char* asset)
{
    u32 strLen = (u32)strlen(asset);
    u32 strBytes = strLen + 1;

    u32 msgSize = strBytes + 8;
    u8* data = (u8*)malloc(msgSize);

    u8* ptr = data;
    ptr = Write(ptr, MSG_ASSET_COMPILED);
    ptr = Write(ptr, strLen);
    ptr = Write(ptr, asset, strLen + 1);

    Message message;
    message.size = msgSize;
    message.bytes = data;

    {
        std::lock_guard<std::mutex> lock(m_messageQueueMutex);
        bool empty = m_messageQueue.empty();
        m_messageQueue.push(message);
        if (empty)
            m_condVar.notify_all();
    }
}

void AssetEventService::ThreadProc()
{
    TcpSocket serverSocket;
    if (!serverSocket.Bind(LOCALHOST, PORT))
        FATAL("Failed to bind socket");
    if (!serverSocket.Listen(0))
        FATAL("Failed to setup socket listening");

    for (;;) {
        TcpSocket socket;
        if (!serverSocket.Accept(&socket))
            FATAL("Failed to accept socket");

        for (;;) {
            {
                std::unique_lock<std::mutex> lock(m_messageQueueMutex);
                m_condVar.wait(lock, [=] { return !m_messageQueue.empty(); });
            }
            if (!SendQueuedMessages(socket))
                break;
        }
    }
}

bool AssetEventService::SendQueuedMessages(TcpSocket& socket)
{
    for (;;) {
        Message message;
        {
            std::lock_guard<std::mutex> lock(m_messageQueueMutex);
            if (m_messageQueue.empty())
                break;
            message = m_messageQueue.front();
            m_messageQueue.pop();
        }

        u32 swappedSize = EndianSwapLE32(message.size);
        if (!socket.Send(&swappedSize, sizeof swappedSize, NULL))
            return false;
        if (!socket.Send(message.bytes, message.size, NULL))
            return false;
    }

    return true;
}
