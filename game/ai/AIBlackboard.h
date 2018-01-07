#pragma once

#include <vector>
#include "lua/LuaRef.h"

class GameObject;

class AIBlackboard
{
public:
    AIBlackboard(GameObject *game_object);
    virtual ~AIBlackboard();

    void SetGameObjectMeta(LuaRef &&game_object_meta);
    void RefTreeNode(lua_State *L, int index);

    GameObject *game_object() const { return game_object_; }
    const LuaRef &game_object_meta() const { return game_object_meta_; }

private:
    GameObject * const game_object_;
    LuaRef game_object_meta_;
    std::vector<LuaRef> tree_node_list_;
};
