#import <Foundation/Foundation.h>
#import "IPCConnectionDelegate.h"

@interface IPCConnection : NSObject

- (void)listenAsServerOnPort:(uint16_t)port;
- (void)connectAsClientToPort:(uint16_t)port;

- (uint16_t)port;

- (void)sendByte:(uint8_t)byte;
- (void)sendByte:(uint8_t)byte onComplete:(nonnull void (^)(void))block;

@property (weak, nullable) id<IPCConnectionDelegate> delegate;

@end
