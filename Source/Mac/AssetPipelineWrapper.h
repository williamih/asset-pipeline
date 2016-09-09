#import <Foundation/Foundation.h>
#import "AssetPipelineMacDelegate.h"

@class ProjectDBConnWrapper;

@interface AssetPipelineWrapper : NSObject

- (nonnull instancetype )init __attribute__((unavailable("Use -initWithDBConn:")));
- (nonnull instancetype)initWithDBConn:(nonnull ProjectDBConnWrapper *)dbConn NS_DESIGNATED_INITIALIZER;
- (void)setProjectWithDirectory:(nonnull NSString *)path;
- (void)compile;
- (void)pollMessages;

@property (weak, nullable) id<AssetPipelineMacDelegate> delegate;

@end
