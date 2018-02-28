#include "AIControlNode.h"
#include "AIBlackboard.h"

AIControlNode::AIControlNode(AIBlackboard &blackboard)
: AINodeBase(blackboard)
{
}

AIControlNode::~AIControlNode()
{
}

int AIControlNode::AppendChildNode(lua_State *L)
{
    children_node_.push_back(lua::read<AINodeBase*>::invoke(L, 2));
    blackboard_.RefTreeNode(L, 2);
    return 0;
}

AIPrioritySelector::AIPrioritySelector(AIBlackboard &blackboard)
: AIControlNode(blackboard)
, current_selected_index_(-1)
{
}

AIPrioritySelector::~AIPrioritySelector()
{
}

AINodeBase::Status AIPrioritySelector::Run()
{
    status_ = children_node_[current_selected_index_]->Run();
    return status_;
}

void AIPrioritySelector::OnInterrupt()
{
    children_node_[current_selected_index_]->Interrupt();
    current_selected_index_ = -1;
}

void AIPrioritySelector::OnFinish()
{
    children_node_[current_selected_index_]->Finish();
    current_selected_index_ = -1;
}

bool AIPrioritySelector::InternalEvaluate()
{
    for (size_t index = 0, total = children_node_.size(); index < total; ++index) {
        switch (children_node_[index]->Evaluate()) {
        case Runnable:
            SelectChildNode(index);
            return true;
        case Reachable:
            SelectChildNode(index);
            return false;
        default:
            break;
        }
    }
    return false;
}

void AIPrioritySelector::SelectChildNode(ssize_t evaluate_selected_index)
{
    if (current_selected_index_ > evaluate_selected_index) {
        children_node_[current_selected_index_]->Interrupt();
    }
    current_selected_index_ = evaluate_selected_index;
}
