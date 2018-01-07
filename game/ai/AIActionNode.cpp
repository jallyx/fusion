#include "AIActionNode.h"

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
