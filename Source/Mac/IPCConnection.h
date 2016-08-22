#import <Foundation/Foundation.h>
#import "IPCConnectionDelegate.h"

@interface IPCConnection : NSObject

- (void)listenAsServerOnPort:(uint16_t)port;
- (void)connectAsClientToPort:(uint16_t)port;

- (uint16_t)port;

- (void)sendByte:(uint8_t)byte;

@property (weak, nullable) id<IPCConnectionDelegate> delegate;

@end
