#include "AIObject.h"
#include "AIActionNode.h"
#include "AIControlNode.h"
#include "lua/LuaTable.h"
#include "lua/LuaMgr.h"
#include "Exception.h"

AIObject::AIObject()
{
}

AIObject::~AIObject()
{
}

void AIObject::BuildBehaviorTree(lua_State *L, const char *aifile)
{
    AIBlackboard &blackboard = GetBlackboard();
    blackboard.RefTreeNode = std::bind(&AIBehaviorTree::RefTreeNode,
        &behavior_tree_, std::placeholders::_1, std::placeholders::_2);

    LuaMgr::DoFileEnv(L, aifile);
    lua::call<void, const LuaRef &>(L, "BuildBehaviorTree", blackboard.object_cache());
}

void AIObject::RunBehaviorTree()
{
    TRY_BEGIN {
        if (behavior_tree_.IsRunnable()) {
            behavior_tree_.Run();
        }
    } TRY_END
    CATCH_BEGIN(const IException &e) {
        e.Print();
    } CATCH_END
    CATCH_BEGIN(...) {
    } CATCH_END
}

void AIObject::InterruptBehaviorTree()
{
    TRY_BEGIN {
        behavior_tree_.Interrupt();
    } TRY_END
    CATCH_BEGIN(const IException &e) {
        e.Print();
    } CATCH_END
    CATCH_BEGIN(...) {
    } CATCH_END
}

void AIObject::InitBehaviorTree(lua_State *L)
{
    LuaTable t(L, "AINodeStatus");
    t.set("Idle", AINodeBase::Idle);
    t.set("Running", AINodeBase::Running);
    t.set("Finished", AINodeBase::Finished);

    lua::class_add<AIObject>(L, "AIObject");
    lua::class_def<AIObject>(L, "GetBlackboard", &AIObject::GetBlackboard);
    lua::class_mem<AIObject>(L, "Tree", &AIObject::behavior_tree_);

    lua::class_add<AIBehaviorTree>(L, "AIBehaviorTree");
    lua::class_def<AIBehaviorTree>(L, "SetRootNode", &AIBehaviorTree::SetRootNode);

    lua::class_add<AIBlackboard>(L, "AIBlackboard");
    lua::class_def<AIBlackboard>(L, "object", &AIBlackboard::object_cache);

    lua::class_add<AINodeBase>(L, "AINodeBase");
    lua::class_def<AINodeBase>(L, "SetExternalPrecondition", &AINodeBase::SetExternalPrecondition);

    lua::class_add<AIActionNode>(L, "AIActionNode");
    lua::class_inh<AIActionNode, AINodeBase>(L);

    lua::class_add<AIInstructionNode>(L, "AIInstructionNode");
    lua::class_con<AIInstructionNode>(L, lua::constructor<AIInstructionNode, AIBlackboard&>);
    lua::class_def<AIInstructionNode>(L, "SetInstructions", &AIInstructionNode::SetInstructions);
    lua::class_inh<AIInstructionNode, AIActionNode>(L);

    lua::class_add<AIScriptableNode>(L, "AIScriptableNode");
    lua::class_con<AIScriptableNode>(L, lua::constructor<AIScriptableNode, AIBlackboard&>);
    lua::class_def<AIScriptableNode>(L, "SetInternalPrecondition", &AIScriptableNode::SetInternalPrecondition);
    lua::class_def<AIScriptableNode>(L, "SetKernelScriptable", &AIScriptableNode::SetKernelScriptable);
    lua::class_def<AIScriptableNode>(L, "SetPrepareScriptable", &AIScriptableNode::SetPrepareScriptable);
    lua::class_def<AIScriptableNode>(L, "SetInterruptScriptable", &AIScriptableNode::SetInterruptScriptable);
    lua::class_def<AIScriptableNode>(L, "SetFinishScriptable", &AIScriptableNode::SetFinishScriptable);
    lua::class_inh<AIScriptableNode, AIActionNode>(L);

    lua::class_add<AIControlNode>(L, "AIControlNode");
    lua::class_def<AIControlNode>(L, "AppendChildNode", &AIControlNode::AppendChildNode);
    lua::class_inh<AIControlNode, AINodeBase>(L);

    lua::class_add<AIPrioritySelector>(L, "AIPrioritySelector");
    lua::class_con<AIPrioritySelector>(L, lua::constructor<AIPrioritySelector, AIBlackboard&>);
    lua::class_inh<AIPrioritySelector, AIControlNode>(L);

    lua::class_add<AISequenceSelector>(L, "AISequenceSelector");
    lua::class_con<AISequenceSelector>(L, lua::constructor<AISequenceSelector, AIBlackboard&>);
    lua::class_inh<AISequenceSelector, AIControlNode>(L);

    lua::class_add<AIWeightedSelector>(L, "AIWeightedSelector");
    lua::class_con<AIWeightedSelector>(L, lua::constructor<AIWeightedSelector, AIBlackboard&>);
    lua::class_def<AIWeightedSelector>(L, "AppendChildNode", &AIWeightedSelector::AppendChildNode);
    lua::class_inh<AIWeightedSelector, AIControlNode>(L);

    lua::class_add<AISequenceNode>(L, "AISequenceNode");
    lua::class_con<AISequenceNode>(L, lua::constructor<AISequenceNode, AIBlackboard&>);
    lua::class_def<AISequenceNode>(L, "SetLoopParams", &AISequenceNode::SetLoopParams);
    lua::class_inh<AISequenceNode, AIControlNode>(L);

    lua::class_add<AIParallelNode>(L, "AIParallelNode");
    lua::class_con<AIParallelNode>(L, lua::constructor<AIParallelNode, AIBlackboard&>);
    lua::class_def<AIParallelNode>(L, "SetParallelParams", &AIParallelNode::SetParallelParams);
    lua::class_inh<AIParallelNode, AIControlNode>(L);
}
