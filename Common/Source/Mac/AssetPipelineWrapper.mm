#import "AssetPipelineWrapper.h"

#include <Core/Macros.h>
#include <Pipeline/AssetPipeline.h>

#import "ProjectDBConnWrapper.h"

class AssetPipelineCppDelegateMac : public AssetPipelineDelegate {
public:
    virtual void OnAssetBuildFinished(unsigned projectIndex, int nSucceeded, int nFailed)
    {
        ASSERT(objCDelegate);
        NSUInteger projIndex = (NSUInteger)projectIndex;
        NSInteger successCount = (NSInteger)nSucceeded;
        NSInteger failureCount = (NSInteger)nFailed;
        [objCDelegate assetBuildFinishedWithProjectIndex:projIndex
                                            successCount:successCount
                                            failureCount:failureCount];
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

- (void)compileProjectWithIndex:(NSUInteger)index {
    _cppAssetPipeline->CompileProject((unsigned)index);
}

- (void)pollMessages {
    if (self.delegate == nil)
        return;

    _cppDelegate->objCDelegate = self.delegate;
    _cppAssetPipeline->CallDelegateFunctions();
}

@end
