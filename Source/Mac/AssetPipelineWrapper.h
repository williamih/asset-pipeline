#import <Foundation/Foundation.h>
#import "AssetPipelineMacDelegate.h"

@class ProjectDBConnWrapper;

@interface AssetPipelineWrapper : NSObject

- (void)setProjectWithDirectory:(nonnull NSString *)path;
- (void)compile;
- (void)pollMessages;

@property (weak, nullable) id<AssetPipelineMacDelegate> delegate;

@end
