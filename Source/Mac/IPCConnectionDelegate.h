#import <Foundation/Foundation.h>

@protocol IPCConnectionDelegate <NSObject>

@required

- (void)receiveIPCByte:(uint8_t)byte;

@end
