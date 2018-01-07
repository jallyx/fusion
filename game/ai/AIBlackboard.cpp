#include "AIBlackboard.h"

AIBlackboard::AIBlackboard(GameObject *game_object)
: game_object_(game_object)
{
}

AIBlackboard::~AIBlackboard()
{
}

void AIBlackboard::SetGameObjectMeta(LuaRef &&game_object_meta)
{
    game_object_meta_ = std::move(game_object_meta);
}

void AIBlackboard::RefTreeNode(lua_State *L, int index)
{
    tree_node_list_.emplace_back(L, index);
}
