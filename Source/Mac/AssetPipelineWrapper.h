#import <Foundation/Foundation.h>
#import "AssetPipelineMacDelegate.h"

@class ProjectDBConnWrapper;

@interface AssetPipelineWrapper : NSObject

- (void)compileProjectWithIndex:(NSUInteger)index;
- (void)pollMessages;

@property (weak, nullable) id<AssetPipelineMacDelegate> delegate;

@end
