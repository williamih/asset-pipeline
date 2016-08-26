#import <Foundation/Foundation.h>
#import "AssetPipelineDelegate.h"

@interface AssetPipelineWrapper : NSObject

- (void)setProjectWithDirectory:(nonnull NSString *)path;
- (void)compile;
- (void)pollMessages;

@property (weak, nullable) id<AssetPipelineDelegate> delegate;

@end
