#ifndef PIPELINE_ASSETPIPELINE_H
#define PIPELINE_ASSETPIPELINE_H

struct lua_State;

class AssetPipeline {
public:
    AssetPipeline();
    ~AssetPipeline();

private:
    AssetPipeline(const AssetPipeline&);
    AssetPipeline& operator=(const AssetPipeline&);

    lua_State* m_luaState;
};

#endif // PIPELINE_ASSETPIPELINE_H
