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

- (void)pollMessages {
    if (self.delegate == nil)
        return;

    AssetPipelineMessage msg;
    while (_cppAssetPipeline->PollMessage(&msg)) {
        switch (msg.type) {
            case ASSET_PIPELINE_COMPILE_FINISHED:
                [self.delegate assetCompileFinishedWithFileCount:msg.intValue];
                break;
            default:
                break;
        }
    }
}

@end
