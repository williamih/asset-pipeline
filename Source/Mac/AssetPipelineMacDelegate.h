#import <Foundation/Foundation.h>

@protocol AssetPipelineMacDelegate <NSObject>

@required

- (void)assetBuildFinishedWithProjectIndex:(NSUInteger)projIndex
                              successCount:(NSInteger)successCount
                              failureCount:(NSInteger)failureCount;
- (void)assetCompileFailedWithInputPaths:(nonnull NSArray<NSString *> *)inputPaths
                             outputPaths:(nonnull NSArray<NSString *> *)outputPaths
                            errorMessage:(nonnull NSString *)errorMessage;

@end
