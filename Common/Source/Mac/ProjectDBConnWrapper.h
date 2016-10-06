#import <Foundation/Foundation.h>

@interface ProjectDBConnWrapper : NSObject

- (NSInteger)numProjects;
- (nonnull NSArray<NSNumber *> *)queryAllProjectIDs;
- (nonnull NSString *)nameOfProjectWithID:(NSInteger)projID;
- (nonnull NSString *)directoryOfProjectWithID:(NSInteger)projID;

- (void)addProjectWithName:(nonnull NSString *)name directory:(nonnull NSString *)directory;

- (NSInteger)activeProjectID;
- (void)setActiveProjectID:(NSInteger)projID;

@end
