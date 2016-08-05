#include "AssetPipeline.h"
#include <lua.hpp>

AssetPipeline::AssetPipeline()
    : m_luaState(luaL_newstate())
{}

AssetPipeline::~AssetPipeline()
{
    lua_close(m_luaState);
}
