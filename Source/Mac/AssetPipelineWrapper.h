#import <Foundation/Foundation.h>
#import "AssetPipelineMacDelegate.h"

@interface AssetPipelineWrapper : NSObject

- (void)setProjectWithDirectory:(nonnull NSString *)path;
- (void)compile;
- (void)pollMessages;

@property (weak, nullable) id<AssetPipelineMacDelegate> delegate;

@end
