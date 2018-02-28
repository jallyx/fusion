#pragma once

#include "AIBehaviorTree.h"
#include "AIBlackboard.h"

class AIObject : public lua::binder, public noncopyable
{
public:
    AIObject();
    ~AIObject();

    void BuildBehaviorTree(lua_State *L, const char *aifile);
    void RunBehaviorTree();
    void InterruptBehaviorTree();

    static void InitBehaviorTree(lua_State *L);

protected:
    virtual AIBlackboard &GetBlackboard() = 0;

private:
    AIBehaviorTree behavior_tree_;
};
