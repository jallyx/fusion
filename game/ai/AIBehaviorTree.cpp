#include "AIBehaviorTree.h"
#include "AIActionNode.h"
#include "AIControlNode.h"

AIBehaviorTree::AIBehaviorTree(AIBlackboard &blackboard)
: blackboard_(blackboard)
, root_node_(nullptr)
{
}

AIBehaviorTree::~AIBehaviorTree()
{
}

int AIBehaviorTree::SetRootNode(lua_State *L)
{
    root_node_ = lua::read<AINodeBase*>::invoke(L, 2);
    blackboard_.RefTreeNode(L, 2);
    return 0;
}

void AIBehaviorTree::Update()
{
    if (root_node_->Evaluate()) {
        if (root_node_->Run() == AINodeBase::Finished) {
            root_node_->Finish();
        }
    }
}

void AIBehaviorTree::InitBehaviorTree(lua_State *L)
{
    lua::class_add<AIBehaviorTree>(L, "AIBehaviorTree");
    lua::class_def<AIBehaviorTree>(L, "SetRootNode", &AIBehaviorTree::SetRootNode);
    lua::class_def<AIBehaviorTree>(L, "blackboard", &AIBehaviorTree::blackboard);

    lua::class_add<AINodeBase>(L, "AINodeBase");
    lua::class_def<AINodeBase>(L, "SetExternalPrecondition", &AINodeBase::SetExternalPrecondition);

    lua::class_add<AIActionNode>(L, "AIActionNode");
    lua::class_inh<AIActionNode, AINodeBase>(L);

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
