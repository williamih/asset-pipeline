#include "Os/TcpSocket.h"

#include <string.h> // for memset
#include <unistd.h> // for close()
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>

#include "Core/Macros.h"

const u32 FLAG_NONBLOCKING = 1;
const u32 FLAG_LISTENING = 2;

static void ProcessSocketError(int error)
{
    switch (error) {
        // Errors that should not occur -- indicating a programming error in
        // this file.
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
        case EBADF:
        case EDESTADDRREQ:
        case EMSGSIZE:
        case ENOTCONN:
        case ENOTSOCK:
        case EOPNOTSUPP:
        case EPIPE:
        case EALREADY:
        case EINPROGRESS:
        case EISCONN:
        case EINVAL:
        case ELOOP:
        case ENAMETOOLONG: // not sure if this error should be in this category?
        case EFAULT:
        // Errors relating to AF_UNIX sockets -- we don't use these, so these
        // errors should not occur
        case ENOENT:
        case ENOTDIR:
            FATAL("TcpSocket: programming error: %s", strerror(error));
            break;

        // Errors relating to the address
        case EAFNOSUPPORT:
            FATAL("TcpSocket: address invalid or unavailable");
            break;

        case EADDRNOTAVAIL:
            break;

        // Too many file descriptors open
        case EMFILE:
        case ENFILE:
            break;

        // Errors indicating a problem with the network or system -- these are
        // not under our control.
        case EACCES:
        case EIO:
        case EADDRINUSE:
        case ECONNRESET:
        case ECONNREFUSED:
        case EHOSTUNREACH:
        case ENETDOWN:
        case ENETUNREACH:
        case ENOBUFS:
        case ENOMEM:
        case EPROTOTYPE:
        case ETIMEDOUT:
        case EPROTONOSUPPORT: // not sure whether this is in the right place
            break;

        default:
            FATAL("TcpSocket: error: %s", strerror(error));
            break;
    }
}

TcpSocket::TcpSocket()
    : m_handle(-1)
    , m_flags(0)
{
}

TcpSocket::~TcpSocket()
{
    Disconnect();
}

TcpSocket::OsHandle TcpSocket::GetOsHandle() const
{
    return m_handle;
}

TcpSocket::BlockingMode TcpSocket::GetBlockingMode() const
{
    return (m_flags & FLAG_NONBLOCKING) ? NONBLOCKING : BLOCKING;
}

void TcpSocket::SetBlockingMode(BlockingMode blockingMode)
{
    Create();

    int flags = fcntl(m_handle, F_GETFL, 0);
    if (flags < 0)
        FATAL("fcntl");
    if (blockingMode == NONBLOCKING) {
        flags |= O_NONBLOCK;
        m_flags |= FLAG_NONBLOCKING;
    } else {
        flags &= ~(O_NONBLOCK);
        m_flags &= ~(FLAG_NONBLOCKING);
    }
    if (fcntl(m_handle, F_SETFL, flags) != 0)
        FATAL("fcntl");
}

void TcpSocket::CheckErrorStatus(bool* result)
{
    ASSERT(m_handle != -1);

    int error = 0;
    socklen_t len = sizeof error;
    int retval = getsockopt(m_handle, SOL_SOCKET, SO_ERROR, &error, &len);
    if (retval != 0)
        FATAL("getsockopt: %s", strerror(retval));

    if (error == 0) {
        if (result) *result = true;
    } else {
        if (result) *result = false;
        ProcessSocketError(error);
    }
}

TcpSocket::SocketResult TcpSocket::Connect(u32 address, u16 port)
{
    Create();

    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(address);
    for (;;) {
        if (connect(m_handle, (sockaddr*)&addr, sizeof addr) != 0) {
            if (errno == EINTR)
                continue;
            // TODO: Do we need to check for EWOULDBLOCK as well?
            if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
                return WOULDBLOCK;
            }
            ProcessSocketError(errno);
            Disconnect();
            return FAILURE;
        }
        break;
    }

    return SUCCESS;
}

void TcpSocket::Disconnect()
{
    if (m_handle != -1) {
        if (close(m_handle) == -1)
            FATAL("close");
        m_handle = -1;
    }
}

bool TcpSocket::Send(const void* data, size_t bytes, size_t* sent)
{
    ASSERT(m_handle != -1);

    bool succeeded = true;
    size_t remaining = bytes;
    const u8* p = (const u8*)data;

    while (remaining > 0) {
        ssize_t bytesJustSent = send(m_handle, p, remaining, 0);
        if (bytesJustSent == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            ProcessSocketError(errno);
            Disconnect();
            succeeded = false;
            break;
        }
        remaining -= (size_t)bytesJustSent;
        p = p + (size_t)bytesJustSent;
    }

    if (sent)
        *sent = p - (const u8*)data;

    return succeeded;
}

TcpSocket::SocketResult TcpSocket::Recv(void* buf, size_t bufLen, size_t* received)
{
    ASSERT(m_handle != -1);

    SocketResult result = SUCCESS;
    ssize_t ret;
    for (;;) {
        ret = recv(m_handle, buf, bufLen, 0);
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                result = WOULDBLOCK;
            } else {
                ProcessSocketError(errno);
                Disconnect();
                result = FAILURE;
            }
        }
        break;
    }
    if (ret == 0)
        Disconnect(); // orderly shutdown on remote end
    if (received)
        *received = (result == SUCCESS) ? (size_t)ret : 0;
    return result;
}

void TcpSocket::Create()
{
    if (m_handle == -1) {
        m_handle = socket(PF_INET, SOCK_STREAM, 0);
        if (m_handle == -1)
            FATAL("Failed to create socket: %s", strerror(errno));
    }
}

void TcpSocket::Create(OsHandle handle)
{
    ASSERT(m_handle == -1);
    m_handle = handle;
}

bool TcpSocket::Bind(u32 address, u16 port)
{
    Create();

    sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(address);

    if (bind(m_handle, (sockaddr*)&addr, sizeof addr) == -1) {
        ProcessSocketError(errno);
        Disconnect();
        return false;
    }

    return true;
}

bool TcpSocket::Listen(int backlog)
{
    ASSERT(m_handle != -1);

    if (listen(m_handle, backlog) == -1) {
        ProcessSocketError(errno);
        Disconnect();
        return false;
    }

    m_flags |= FLAG_LISTENING;

    return true;
}

bool TcpSocket::Accept(TcpSocket* socket)
{
    ASSERT(socket);
    ASSERT(m_handle != -1);
    ASSERT(m_flags & FLAG_LISTENING);

    int fd = accept(m_handle, NULL, NULL);
    if (fd == -1) {
        ProcessSocketError(m_handle);
        Disconnect();
        return false;
    }

    socket->Disconnect();
    socket->Create(fd);

    return true;
}
