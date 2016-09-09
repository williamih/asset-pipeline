#import "ProjectDBConnWrapper.h"
#include <Pipeline/ProjectDBConn.h>

@implementation ProjectDBConnWrapper {
    ProjectDBConn* _cppDBConn;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _cppDBConn = new ProjectDBConn;
    }
    return self;
}

- (void)dealloc {
    delete _cppDBConn;
}

- (NSInteger)numProjects {
    return (NSInteger)_cppDBConn->NumProjects();
}

- (nonnull NSString *)nameOfProjectAtIndex:(NSInteger)index {
    NSAssert(index >= 0, @"Negative index");
    std::string str(_cppDBConn->GetProjectName((unsigned)index));
    return [NSString stringWithUTF8String:str.c_str()];
}

- (nonnull NSString *)directoryOfProjectAtIndex:(NSInteger)index {
    NSAssert(index >= 0, @"Negative index");
    std::string str(_cppDBConn->GetProjectDirectory((unsigned)index));
    return [NSString stringWithUTF8String:str.c_str()];
}

- (void)addProjectWithName:(nonnull NSString *)name
                 directory:(nonnull NSString *)directory {
    _cppDBConn->AddProject([name UTF8String], [directory UTF8String]);
}

- (NSInteger)activeProjectIndex {
    return (NSInteger)_cppDBConn->GetActiveProjectIndex();
}

- (void)setActiveProjectIndex:(NSInteger)index {
    NSAssert(index >= -1, @"Invalid index");
    _cppDBConn->SetActiveProjectIndex((int)index);
}

- (nonnull void *)CPlusPlusDBConn {
    return (void *)_cppDBConn;
}

@end
