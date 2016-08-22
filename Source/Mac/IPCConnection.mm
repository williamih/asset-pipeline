#import "IPCConnection.h"
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>

@interface IPCConnection (IPCConnection_Private) <NSStreamDelegate>
@end

@implementation IPCConnection {
    NSInputStream *_readStream;
    NSOutputStream *_writeStream;
    CFSocketRef _serverSocket;
    std::vector<uint8_t> *_byteQueue;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _byteQueue = new std::vector<uint8_t>();
    }
    return self;
}

- (void)dealloc {
    delete _byteQueue;
}

- (void)internalCreateWithInputStream:(NSInputStream *)inputStream
                         outputStream:(NSOutputStream *)outputStream
{
    assert(!_readStream);
    assert(!_writeStream);

    _readStream = inputStream;
    _writeStream = outputStream;

    _readStream.delegate = self;
    _writeStream.delegate = self;

    [_readStream open];
    [_writeStream open];

    [_readStream scheduleInRunLoop:[NSRunLoop currentRunLoop]
                           forMode:NSDefaultRunLoopMode];
    [_writeStream scheduleInRunLoop:[NSRunLoop currentRunLoop]
                            forMode:NSDefaultRunLoopMode];
}

- (void)internalConnectToClientSocket:(CFSocketNativeHandle)clientSocket {
    CFReadStreamRef readStream;
    CFWriteStreamRef writeStream;

    CFStreamCreatePairWithSocket(
        kCFAllocatorDefault,
        clientSocket,
        &readStream,
        &writeStream
    );

    NSInputStream *nsReadStream = CFBridgingRelease(readStream);
    NSOutputStream *nsWriteStream = CFBridgingRelease(writeStream);

    [self internalCreateWithInputStream:nsReadStream outputStream:nsWriteStream];
}

static void SocketCallback(CFSocketRef s, CFSocketCallBackType callbackType,
                           CFDataRef address, const void *data, void *info)
{
    if (callbackType == kCFSocketAcceptCallBack) {
        CFSocketNativeHandle clientSocket = *(CFSocketNativeHandle *)data;
        IPCConnection *conn = (__bridge IPCConnection *)info;
        [conn internalConnectToClientSocket:clientSocket];
    }
}

- (void)listenAsServerOnPort:(uint16_t)port {
    CFSocketContext context;
    context.version = 0;
    context.info = (__bridge void *)self;
    context.retain = NULL;
    context.release = NULL;

    _serverSocket = CFSocketCreate(
        kCFAllocatorDefault,
        PF_INET,
        SOCK_STREAM,
        IPPROTO_TCP,
        kCFSocketAcceptCallBack,
        SocketCallback,
        &context
    );

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;

    CFDataRef addressData = CFDataCreate(
        kCFAllocatorDefault,
        (UInt8 *)&sin,
        sizeof(sin)
    );
    CFSocketSetAddress(_serverSocket, addressData);
    CFRelease(addressData);

    CFRunLoopSourceRef runLoopSource = CFSocketCreateRunLoopSource(
        kCFAllocatorDefault,
        _serverSocket,
        0
    );
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
    CFRelease(runLoopSource);
}

- (void)connectAsClientToPort:(uint16_t)port {
    CFReadStreamRef readStream;
    CFWriteStreamRef writeStream;

    CFStreamCreatePairWithSocketToHost(
        NULL,
        CFSTR("localhost"),
        port,
        &readStream,
        &writeStream
    );

    NSInputStream *nsReadStream = CFBridgingRelease(readStream);
    NSOutputStream *nsWriteStream = CFBridgingRelease(writeStream);

    [self internalCreateWithInputStream:nsReadStream outputStream:nsWriteStream];
}

- (void)sendByte:(uint8_t)byte {
    _byteQueue->push_back(byte);
}

- (uint16_t)port {
    int sockfd = (int)CFSocketGetNative(_serverSocket);
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    if (getsockname(sockfd, (struct sockaddr *)&addr, &len) != 0)
        abort();
    return ntohs(addr.sin_port);
}

- (void)stream:(NSStream *)aStream handleEvent:(NSStreamEvent)eventCode {
    if (aStream == _writeStream && eventCode == NSStreamEventHasSpaceAvailable &&
        !_byteQueue->empty()) {
        //
        const uint8_t *data = &(*_byteQueue)[0];
        NSUInteger maxLength = (NSUInteger)_byteQueue->size();
        NSInteger written = [_writeStream write:data maxLength:maxLength];
        _byteQueue->erase(_byteQueue->begin(),
                          _byteQueue->begin() + (size_t)written);
    }

    if (aStream == _readStream && eventCode == NSStreamEventHasBytesAvailable
        && self.delegate != nil) {
        //
        // TODO: Use a better protocol so that we don't need to just hardcode
        // an arbitrary number.
        const int MAX_BYTES = 128;

        uint8_t bytes[MAX_BYTES];
        NSInteger bytesRead = [_readStream read:bytes maxLength:MAX_BYTES];
        for (NSInteger i = 0; i < bytesRead; ++i) {
            [self.delegate receiveIPCByte:bytes[i]];
        }
    }
}

@end
