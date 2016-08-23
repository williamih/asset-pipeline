#import <Foundation/Foundation.h>

@interface ProjectDBConnWrapper : NSObject

- (NSInteger)numProjects;
- (nonnull NSString *)nameOfProjectAtIndex:(NSInteger)index;
- (nonnull NSString *)directoryOfProjectAtIndex:(NSInteger)index;

- (void)addProjectWithName:(nonnull NSString *)name directory:(nonnull NSString *)directory;

- (NSInteger)activeProjectIndex;
- (void)setActiveProjectIndex:(NSInteger)index;

@end
