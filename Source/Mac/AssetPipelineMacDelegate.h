#import <Foundation/Foundation.h>

@protocol AssetPipelineMacDelegate <NSObject>

@required

- (void)assetCompileFinishedWithFileCount:(NSInteger)count;

@end
