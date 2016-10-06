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

- (nonnull NSArray<NSNumber *> *)queryAllProjectIDs {
    std::vector<int> vec;
    _cppDBConn->QueryAllProjectIDs(&vec);
    NSMutableArray<NSNumber *> *array = [[NSMutableArray alloc] init];
    for (size_t i = 0; i < vec.size(); ++i) {
        [array addObject:@(vec[i])];
    }
    return array;
}

- (nonnull NSString *)nameOfProjectWithID:(NSInteger)projID {
    NSAssert(projID >= 0, @"Negative project ID");
    std::string str(_cppDBConn->GetProjectName((int)projID));
    return [NSString stringWithUTF8String:str.c_str()];
}

- (nonnull NSString *)directoryOfProjectWithID:(NSInteger)projID {
    NSAssert(index >= 0, @"Negative project ID");
    std::string str(_cppDBConn->GetProjectDirectory((int)projID));
    return [NSString stringWithUTF8String:str.c_str()];
}

- (void)addProjectWithName:(nonnull NSString *)name
                 directory:(nonnull NSString *)directory {
    _cppDBConn->AddProject([name UTF8String], [directory UTF8String]);
}

- (NSInteger)activeProjectID {
    return (NSInteger)_cppDBConn->GetActiveProjectID();
}

- (void)setActiveProjectID:(NSInteger)projID {
    NSAssert(projID >= -1, @"Invalid project ID");
    _cppDBConn->SetActiveProjectID((int)projID);
}

@end
