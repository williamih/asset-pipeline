#import <Foundation/Foundation.h>
#import "AssetPipelineMacDelegate.h"

@class ProjectDBConnWrapper;

@interface AssetPipelineWrapper : NSObject

- (void)compileProjectWithID:(NSInteger)projID;
- (void)pollMessages;

@property (weak, nullable) id<AssetPipelineMacDelegate> delegate;

@end
