#import "AssetPipelineWrapper.h"

#include <Pipeline/AssetPipeline.h>

@implementation AssetPipelineWrapper {
    AssetPipeline* _cppAssetPipeline;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _cppAssetPipeline = new AssetPipeline;
    }
    return self;
}

- (void)dealloc {
    delete _cppAssetPipeline;
}

- (void)setProjectWithDirectory:(nonnull NSString *)path {
    _cppAssetPipeline->SetProjectWithDirectory([path UTF8String]);
}

- (void)compile {
    _cppAssetPipeline->Compile();
}

@end
