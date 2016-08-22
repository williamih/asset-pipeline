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
    std::vector<void (^)(void)> *_blockQueue;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _byteQueue = new std::vector<uint8_t>();
        _blockQueue = new std::vector<void (^)(void)>();
    }
    return self;
}

- (void)closeAllStreams {
    [_readStream close];
    [_writeStream close];
    _readStream = nil;
    _writeStream = nil;
    if (_serverSocket != NULL) {
        CFSocketInvalidate(_serverSocket);
        CFRelease(_serverSocket);
        _serverSocket = NULL;
    }
}

- (void)dealloc {
    [self closeAllStreams];
    delete _byteQueue;
    delete _blockQueue;
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

    [_readStream scheduleInRunLoop:[NSRunLoop mainRunLoop]
                           forMode:NSDefaultRunLoopMode];
    [_writeStream scheduleInRunLoop:[NSRunLoop mainRunLoop]
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
    NSAssert(_readStream == nil && _writeStream == nil && _serverSocket == NULL,
             @"IPCConnection already set up");

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
    NSAssert(_readStream == nil && _writeStream == nil && _serverSocket == NULL,
             @"IPCConnection already set up");

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

- (void)internalSendBufferedData {
    if (_byteQueue->empty())
        return;

    const uint8_t *data = &(*_byteQueue)[0];
    NSUInteger maxLength = (NSUInteger)_byteQueue->size();
    NSInteger written = [_writeStream write:data maxLength:maxLength];
    _byteQueue->erase(_byteQueue->begin(),
                      _byteQueue->begin() + (size_t)written);
    for (NSInteger i = 0; i < written; ++i) {
        if ((*_blockQueue)[i] != NULL)
            ((*_blockQueue)[i])();
    }
    _blockQueue->erase(_blockQueue->begin(),
                       _blockQueue->begin() + (size_t)written);
}

- (void)internalSendByte:(uint8_t)byte onComplete:(void (^)(void))block {
    NSAssert([self isSendAvailable], @"IPCConnection is not available for sending");

    _byteQueue->push_back(byte);
    _blockQueue->push_back(block);
    if ([_writeStream hasSpaceAvailable])
        [self internalSendBufferedData];
}

- (void)sendByte:(uint8_t)byte {
    [self internalSendByte:byte onComplete:NULL];
}

- (void)sendByte:(uint8_t)byte onComplete:(nonnull void (^)(void))block {
    [self internalSendByte:byte onComplete:block];
}

- (BOOL)isConnected {
    return _readStream != nil;
}

- (BOOL)isSendAvailable {
    return (_writeStream != nil) || (_serverSocket != NULL);
}

- (uint16_t)port {
    NSAssert(_serverSocket != NULL, @"Must be a server connection");

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
        [self internalSendBufferedData];
    }

    bool connectionClosed = (eventCode == NSStreamEventErrorOccurred) ||
                            (eventCode == NSStreamEventEndEncountered);

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
        if (bytesRead == 0) {
            connectionClosed = true;
        }
    }

    if (connectionClosed) {
        [self closeAllStreams];
        [self.delegate onIPCConnectionClosed];
    }
}

@end
