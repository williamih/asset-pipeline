#import "AssetPipelineWrapper.h"

#include <Core/Macros.h>
#include <Pipeline/AssetPipeline.h>

#import "ProjectDBConnWrapper.h"

class AssetPipelineCppDelegateMac : public AssetPipelineDelegate {
public:
    virtual void OnAssetBuildFinished(int projectID, int nSucceeded, int nFailed)
    {
        ASSERT(objCDelegate);
        [objCDelegate assetBuildFinishedWithProjectID:(NSInteger)projectID
                                         successCount:(NSInteger)nSucceeded
                                         failureCount:(NSInteger)nFailed];
    }

    virtual void OnAssetFailedToCompile(const AssetCompileFailureInfo& info)
    {
        ASSERT(objCDelegate);
        NSMutableArray* inputPaths = [[NSMutableArray alloc] init];
        for (std::size_t i = 0; i < info.inputPaths.size(); ++i) {
            const char* str = info.inputPaths[i].c_str();
            [inputPaths addObject:[NSString stringWithUTF8String:str]];
        }
        NSMutableArray* outputPaths = [[NSMutableArray alloc] init];
        for (std::size_t i = 0; i < info.outputPaths.size(); ++i) {
            const char* str = info.outputPaths[i].c_str();
            [outputPaths addObject:[NSString stringWithUTF8String:str]];
        }
        NSString* errMsg = [NSString stringWithUTF8String:info.errorMessage.c_str()];
        [objCDelegate assetCompileFailedWithInputPaths:inputPaths
                                           outputPaths:outputPaths
                                          errorMessage:errMsg];
    }

    id<AssetPipelineMacDelegate> objCDelegate;
};

@implementation AssetPipelineWrapper {
    AssetPipeline* _cppAssetPipeline;
    AssetPipelineCppDelegateMac* _cppDelegate;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _cppAssetPipeline = new AssetPipeline;
        _cppDelegate = new AssetPipelineCppDelegateMac;
        _cppAssetPipeline->SetDelegate(_cppDelegate);
    }
    return self;
}

- (void)dealloc {
    ASSERT(_cppAssetPipeline->GetDelegate() == _cppDelegate);
    delete _cppAssetPipeline;
    delete _cppDelegate;
}

- (void)compileProjectWithID:(NSInteger)projID {
    _cppAssetPipeline->CompileProject((int)projID);
}

- (void)pollMessages {
    if (self.delegate == nil)
        return;

    _cppDelegate->objCDelegate = self.delegate;
    _cppAssetPipeline->CallDelegateFunctions();
}

@end
