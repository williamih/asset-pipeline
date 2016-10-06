#include "AssetEventService.h"

#include <stdlib.h>
#include <string.h>

#include <Core/Endian.h>
#include <Core/Macros.h>
#include <Os/TcpSocket.h>
#include <Os/SocketOsFunctions.h>

// TODO: There is quite possibly a cleaner way to cleanup the thread in the
// destructor, other than using a timeout for accept().

static u32 Address(u32 a, u32 b, u32 c, u32 d)
{
    return (a << 24) | (b << 16) | (c << 8) | d;
}

const u32 MSG_ASSET_COMPILED = 1;

const u32 PORT = 6789;
const u32 LOCALHOST = Address(127, 0, 0, 1);

const unsigned SOCKET_ACCEPT_TIMEOUT_MS = 20;

AssetEventService::AssetEventService()
    : m_thread()
    , m_messageQueueMutex()
    , m_messageQueue()
    , m_shouldExit(false)
    , m_condVar()
{
    m_thread = std::thread(&AssetEventService::ThreadProc, this);
}

AssetEventService::~AssetEventService()
{
    m_shouldExit = true;
    m_condVar.notify_all();

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
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket.GetOsHandle(), &readSet);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = SOCKET_ACCEPT_TIMEOUT_MS * 1000;

        int result = select(serverSocket.GetOsHandle() + 1, &readSet,
                            NULL, NULL, &tv);
        if (result == -1)
            FATAL("select");
        if (result == 0) {
            if (m_shouldExit)
                break;
            else
                continue;
        }

        TcpSocket socket;
        if (!serverSocket.Accept(&socket))
            FATAL("Failed to accept socket");

        for (;;) {
            {
                std::unique_lock<std::mutex> lock(m_messageQueueMutex);
                m_condVar.wait(lock, [=] {
                    return m_shouldExit || !m_messageQueue.empty();
                });
            }
            if (m_shouldExit)
                break;
            if (!SendQueuedMessages(socket))
                break;
        }
        if (m_shouldExit)
            break;
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
