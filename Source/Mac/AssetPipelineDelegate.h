#import <Foundation/Foundation.h>

@protocol AssetPipelineDelegate <NSObject>

@required

- (void)assetCompileFinishedWithFileCount:(NSInteger)count;

@end
