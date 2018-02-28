#include "AIControlNode.h"
#include <algorithm>
#include <numeric>
#include "AIBlackboard.h"
#include "System.h"

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

AISequenceSelector::AISequenceSelector(AIBlackboard &blackboard)
: AIControlNode(blackboard)
, current_selected_index_(0)
{
}

AISequenceSelector::~AISequenceSelector()
{
}

AINodeBase::Status AISequenceSelector::Run()
{
    status_ = children_node_[current_selected_index_]->Run();
    return status_;
}

void AISequenceSelector::OnInterrupt()
{
    children_node_[current_selected_index_]->Interrupt();
    current_selected_index_ += 1;
}

void AISequenceSelector::OnFinish()
{
    children_node_[current_selected_index_]->Finish();
    current_selected_index_ += 1;
}

bool AISequenceSelector::InternalEvaluate()
{
    const size_t limits[2][2] = {
        {current_selected_index_, children_node_.size()},
        {0, current_selected_index_},
    };
    for (int i = 0; i < 2; ++i) {
        for (size_t index = limits[i][0], end = limits[i][1]; index < end; ++index) {
            switch (children_node_[index]->Evaluate()) {
            case Runnable:
                current_selected_index_ = index;
                return true;
            case Reachable:
                current_selected_index_ = index;
                return false;
            default:
                break;
            }
        }
    }
    return false;
}

AIWeightedSelector::AIWeightedSelector(AIBlackboard &blackboard)
: AIControlNode(blackboard)
, current_selected_index_(-1)
{
}

AIWeightedSelector::~AIWeightedSelector()
{
}

int AIWeightedSelector::AppendChildNode(lua_State *L)
{
    children_weight_.push_back(lua::read<int>::invoke(L, 3));
    return AIControlNode::AppendChildNode(L);
}

AINodeBase::Status AIWeightedSelector::Run()
{
    status_ = children_node_[current_selected_index_]->Run();
    return status_;
}

void AIWeightedSelector::OnInterrupt()
{
    children_node_[current_selected_index_]->Interrupt();
    current_selected_index_ = -1;
}

void AIWeightedSelector::OnFinish()
{
    children_node_[current_selected_index_]->Finish();
    current_selected_index_ = -1;
}

bool AIWeightedSelector::InternalEvaluate()
{
    if (current_selected_index_ != -1) {
        switch (children_node_[current_selected_index_]->Evaluate()) {
        case Runnable: return true;
        case Reachable: return false;
        default: break;
        }
    }

    std::vector<int> children_weight = children_weight_;
    if (current_selected_index_ != -1) {
        children_weight[current_selected_index_] = 0;
    }

    int total_weight = std::accumulate(children_weight.begin(), children_weight.end(), 0);
    for (size_t count = 0, n = children_weight.size(); count < n; ++count) {
        int odds_weight = System::Rand(0, total_weight);
        for (size_t index = 0; index < n; ++index) {
            int &child_weight = children_weight[index];
            if ((odds_weight -= child_weight) < 0) {
                switch (children_node_[index]->Evaluate()) {
                case Runnable:
                    current_selected_index_ = index;
                    return true;
                case Reachable:
                    current_selected_index_ = index;
                    return false;
                default:
                    break;
                }
                total_weight -= child_weight;
                child_weight = 0;
                break;
            }
        }
    }

    return false;
}

AISequenceNode::AISequenceNode(AIBlackboard &blackboard)
: AIControlNode(blackboard)
, loop_total_(1)
, loop_count_(0)
, current_selected_index_(0)
{
}

AISequenceNode::~AISequenceNode()
{
}

void AISequenceNode::SetLoopParams(size_t loop_total)
{
    loop_total_ = std::max(loop_total, (size_t)1);
}

AINodeBase::Status AISequenceNode::Run()
{
    AINodeBase *child_node = children_node_[current_selected_index_];
    if ((status_ = child_node->Run()) == Finished) {
        child_node->Finish();
        if (++current_selected_index_ < children_node_.size()) {
            status_ = Running;
        } else if (++loop_count_ < loop_total_) {
            status_ = Running;
            current_selected_index_ = 0;
        }
    }
    return status_;
}

void AISequenceNode::OnInterrupt()
{
    children_node_[current_selected_index_]->Interrupt();
    current_selected_index_ = 0;
    loop_count_ = 0;
}

void AISequenceNode::OnFinish()
{
    current_selected_index_ = 0;
    loop_count_ = 0;
}

bool AISequenceNode::InternalEvaluate()
{
    switch (children_node_[current_selected_index_]->Evaluate()) {
    case Runnable: return true;
    default: return false;
    }
}

AIParallelNode::AIParallelNode(AIBlackboard &blackboard)
: AIControlNode(blackboard)
, x_as_passed_(SIZE_MAX)
, x_as_finished_(SIZE_MAX)
{
}

AIParallelNode::~AIParallelNode()
{
}

void AIParallelNode::SetParallelParams(size_t x_as_passed, size_t x_as_finished)
{
    x_as_passed_ = x_as_passed;
    x_as_finished_ = x_as_finished;
}

AINodeBase::Status AIParallelNode::Run()
{
    size_t total_finished = 0;
    for (auto index : current_selected_index_) {
        AINodeBase *child_node = children_node_[index];
        if (child_node->status() == Finished || child_node->Run() == Finished) {
            ++total_finished;
        }
    }
    if (total_finished >= std::min(x_as_finished_, children_node_.size())) {
        status_ = Finished;
    } else {
        status_ = Running;
    }
    return status_;
}

void AIParallelNode::OnInterrupt()
{
    for (auto child_node : children_node_) {
        if (child_node->status() == Running) {
            child_node->Interrupt();
        } else if (child_node->status() == Finished) {
            child_node->Finish();
        }
    }
}

void AIParallelNode::OnFinish()
{
    for (auto child_node : children_node_) {
        if (child_node->status() == Running) {
            child_node->Interrupt();
        } else if (child_node->status() == Finished) {
            child_node->Finish();
        }
    }
}

bool AIParallelNode::InternalEvaluate()
{
    size_t reachable_counter = 0;
    current_selected_index_.clear();
    current_selected_index_.reserve(children_node_.size());
    for (size_t index = 0, total = children_node_.size(); index < total; ++index) {
        switch (children_node_[index]->Evaluate()) {
        case Runnable:
            current_selected_index_.push_back(index);
        case Reachable:
            reachable_counter += 1;
            break;
        default:
            break;
        }
    }
    return reachable_counter >= std::min(x_as_passed_, children_node_.size());
}
