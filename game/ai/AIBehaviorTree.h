#pragma once

#include "AIBlackboard.h"

class AINodeBase;

class AIBehaviorTree
{
public:
    AIBehaviorTree(AIBlackboard &blackboard);
    ~AIBehaviorTree();

    int SetRootNode(lua_State *L);

    void Update();

    template <typename T>
    void SetGameObjectMeta(lua_State *L) {
        lua::push<T*>::invoke(L, dynamic_cast<T*>(blackboard_.game_object()));
        blackboard_.SetGameObjectMeta(LuaRef(L, true));
    }

    static void InitBehaviorTree(lua_State *L);

    AIBlackboard &blackboard() { return blackboard_; }

private:
    AIBlackboard &blackboard_;
    AINodeBase *root_node_;
};
