#import <Foundation/Foundation.h>

@protocol AssetPipelineMacDelegate <NSObject>

@required

- (void)assetBuildFinishedWithProjectID:(NSInteger)projectID
                           successCount:(NSInteger)successCount
                           failureCount:(NSInteger)failureCount;
- (void)assetCompileFailedWithInputPaths:(nonnull NSArray<NSString *> *)inputPaths
                             outputPaths:(nonnull NSArray<NSString *> *)outputPaths
                            errorMessage:(nonnull NSString *)errorMessage;

@end
