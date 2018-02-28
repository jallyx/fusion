#pragma once

#include "AINodeBase.h"

class AIActionNode : public AINodeBase
{
public:
    AIActionNode(AIBlackboard &blackboard);
    virtual ~AIActionNode();

    virtual Status Run();

protected:
    virtual Status Kernel() = 0;

    virtual void OnPrepare() {}
};

class AIInstructionNode : public AIActionNode
{
public:
    AIInstructionNode(AIBlackboard &blackboard);
    virtual ~AIInstructionNode();

    int SetInstructions(lua_State *L);

protected:
    virtual Status Kernel();

private:
    LuaRef instructions_;
};

class AIScriptableNode : public AIActionNode
{
public:
    AIScriptableNode(AIBlackboard &blackboard);
    virtual ~AIScriptableNode();

    int SetInternalPrecondition(lua_State *L);
    int SetKernelScriptable(lua_State *L);
    int SetPrepareScriptable(lua_State *L);
    int SetInterruptScriptable(lua_State *L);
    int SetFinishScriptable(lua_State *L);

protected:
    virtual bool InternalEvaluate();

    virtual Status Kernel();

    virtual void OnPrepare();
    virtual void OnInterrupt();
    virtual void OnFinish();

private:
    LuaRef internal_precondition_;
    LuaRef kernel_scriptable_;
    LuaRef prepare_scriptable_;
    LuaRef interrupt_scriptable_;
    LuaRef finish_scriptable_;
};
