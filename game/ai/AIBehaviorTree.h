#pragma once

#include <vector>
#include "noncopyable.h"
#include "lua/LuaRef.h"

class AINodeBase;

class AIBehaviorTree : public noncopyable
{
public:
    AIBehaviorTree();
    ~AIBehaviorTree();

    int SetRootNode(lua_State *L);

    void RefTreeNode(lua_State *L, int index);

    void Run();
    void Interrupt();

    bool IsRunnable() const { return root_node_ != nullptr; }

private:
    AINodeBase *root_node_;
    std::vector<LuaRef> tree_node_list_;
};
