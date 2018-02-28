#include "AIActionNode.h"
#include "AIBlackboard.h"
#include "lua/LuaFunc.h"

AIActionNode::AIActionNode(AIBlackboard &blackboard)
: AINodeBase(blackboard)
{
}

AIActionNode::~AIActionNode()
{
}

AINodeBase::Status AIActionNode::Run()
{
    if (status_ == Idle) {
        status_ = Running;
        OnPrepare();
    }
    if (status_ == Running) {
        status_ = Kernel();
    }
    return status_;
}

AIScriptableNode::AIScriptableNode(AIBlackboard &blackboard)
: AIActionNode(blackboard)
{
}

AIScriptableNode::~AIScriptableNode()
{
}

int AIScriptableNode::SetInternalPrecondition(lua_State *L)
{
    internal_precondition_ = LuaRef(L, 2);
    return 0;
}

int AIScriptableNode::SetKernelScriptable(lua_State *L)
{
    kernel_scriptable_ = LuaRef(L, 2);
    return 0;
}

int AIScriptableNode::SetPrepareScriptable(lua_State *L)
{
    prepare_scriptable_ = LuaRef(L, 2);
    return 0;
}

int AIScriptableNode::SetInterruptScriptable(lua_State *L)
{
    interrupt_scriptable_ = LuaRef(L, 2);
    return 0;
}

int AIScriptableNode::SetFinishScriptable(lua_State *L)
{
    finish_scriptable_ = LuaRef(L, 2);
    return 0;
}

bool AIScriptableNode::InternalEvaluate()
{
    if (!internal_precondition_.isref())
        return true;
    if (LuaFunc(internal_precondition_).Call<bool, const LuaRef &>(blackboard_.object_cache()))
        return true;
    return false;
}

AINodeBase::Status AIScriptableNode::Kernel()
{
    return LuaFunc(kernel_scriptable_).Call<Status, const LuaRef &>(blackboard_.object_cache());
}

void AIScriptableNode::OnPrepare()
{
    if (prepare_scriptable_.isref()) {
        LuaFunc(prepare_scriptable_).Call<void, const LuaRef &>(blackboard_.object_cache());
    }
}

void AIScriptableNode::OnInterrupt()
{
    if (interrupt_scriptable_.isref()) {
        LuaFunc(interrupt_scriptable_).Call<void, const LuaRef &>(blackboard_.object_cache());
    }
}

void AIScriptableNode::OnFinish()
{
    if (finish_scriptable_.isref()) {
        LuaFunc(finish_scriptable_).Call<void, const LuaRef &>(blackboard_.object_cache());
    }
}
