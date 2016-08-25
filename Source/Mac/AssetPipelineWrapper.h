#import <Foundation/Foundation.h>

@interface AssetPipelineWrapper : NSObject

- (void)setProjectWithDirectory:(nonnull NSString *)path;
- (void)compile;

@end
