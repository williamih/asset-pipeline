#import "AssetPipelineWrapper.h"

#include <Core/Macros.h>
#include <Pipeline/AssetPipeline.h>

class AssetPipelineCppDelegateMac : public AssetPipelineDelegate {
public:
    virtual void OnAssetBuildFinished(int nCompiledSuccessfully)
    {
        ASSERT(objCDelegate);
        NSInteger count = (NSInteger)nCompiledSuccessfully;
        [objCDelegate assetCompileFinishedWithFileCount:count];
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

- (void)setProjectWithDirectory:(nonnull NSString *)path {
    _cppAssetPipeline->SetProjectWithDirectory([path UTF8String]);
}

- (void)compile {
    _cppAssetPipeline->Compile();
}

- (void)pollMessages {
    if (self.delegate == nil)
        return;

    _cppDelegate->objCDelegate = self.delegate;
    _cppAssetPipeline->CallDelegateFunctions();
}

@end
