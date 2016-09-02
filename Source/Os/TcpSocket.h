#ifndef OS_TCPSOCKET_H
#define OS_TCPSOCKET_H

#include <stddef.h>
#include "Core/Types.h"

class TcpSocket {
public:
    enum BlockingMode {
        BLOCKING,
        NONBLOCKING,
    };

    enum SocketResult {
        SUCCESS,
        FAILURE,
        WOULDBLOCK,
    };

    typedef int OsHandle;

    TcpSocket();
    ~TcpSocket();

    OsHandle GetOsHandle() const;

    BlockingMode GetBlockingMode() const;
    void SetBlockingMode(BlockingMode blockingMode);

    void CheckErrorStatus(bool* result);
    SocketResult Connect(u32 address, u16 port);
    void Disconnect();

    bool Send(const void* data, size_t bytes, size_t* sent);
    SocketResult Recv(void* buf, size_t bufLen, size_t* received);

    bool Bind(u32 address, u16 port);
    bool Listen(int backlog);
    bool Accept(TcpSocket* socket);

private:
    TcpSocket(const TcpSocket&);
    TcpSocket& operator=(const TcpSocket&);

    void Create();
    void Create(OsHandle handle);

    OsHandle m_handle;
    u32 m_flags;
};

#endif // OS_TCPSOCKET_H
