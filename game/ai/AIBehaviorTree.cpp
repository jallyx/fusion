#include "AIBehaviorTree.h"
#include "AINodeBase.h"

AIBehaviorTree::AIBehaviorTree()
: root_node_(nullptr)
{
}

AIBehaviorTree::~AIBehaviorTree()
{
}

int AIBehaviorTree::SetRootNode(lua_State *L)
{
    root_node_ = lua::read<AINodeBase*>::invoke(L, 2);
    RefTreeNode(L, 2);
    return 0;
}

void AIBehaviorTree::RefTreeNode(lua_State *L, int index)
{
    tree_node_list_.emplace_back(L, index);
}

void AIBehaviorTree::Run()
{
    if (root_node_->Evaluate() == AINodeBase::Runnable) {
        if (root_node_->Run() == AINodeBase::Finished) {
            root_node_->Finish();
        }
    }
}

void AIBehaviorTree::Interrupt()
{
    root_node_->Interrupt();
}
