#pragma once

#include "AINodeBase.h"
#include <vector>

class AIControlNode : public AINodeBase
{
public:
    AIControlNode(AIBlackboard &blackboard);
    virtual ~AIControlNode();

    int AppendChildNode(lua_State *L);

protected:
    std::vector<AINodeBase*> children_node_;
};

class AIPrioritySelector : public AIControlNode
{
public:
    AIPrioritySelector(AIBlackboard &blackboard);
    virtual ~AIPrioritySelector();

    virtual Status Run();

protected:
    virtual bool InternalEvaluate();

    virtual void OnInterrupt();
    virtual void OnFinish();

private:
    void SelectChildNode(ssize_t evaluate_selected_index);

    ssize_t current_selected_index_;
};

class AISequenceSelector : public AIControlNode
{
public:
    AISequenceSelector(AIBlackboard &blackboard);
    virtual ~AISequenceSelector();

    virtual Status Run();

protected:
    virtual bool InternalEvaluate();

    virtual void OnInterrupt();
    virtual void OnFinish();

private:
    size_t current_selected_index_;
};

class AIWeightedSelector : public AIControlNode
{
public:
    AIWeightedSelector(AIBlackboard &blackboard);
    virtual ~AIWeightedSelector();

    int AppendChildNode(lua_State *L);

    virtual Status Run();

protected:
    virtual bool InternalEvaluate();

    virtual void OnInterrupt();
    virtual void OnFinish();

private:
    std::vector<int> children_weight_;
    ssize_t current_selected_index_;
};

class AISequenceNode : public AIControlNode
{
public:
    AISequenceNode(AIBlackboard &blackboard);
    virtual ~AISequenceNode();

    void SetLoopParams(size_t loop_total);

    virtual Status Run();

protected:
    virtual bool InternalEvaluate();

    virtual void OnInterrupt();
    virtual void OnFinish();

private:
    size_t loop_total_, loop_count_;
    size_t current_selected_index_;
};

class AIParallelNode : public AIControlNode
{
public:
    AIParallelNode(AIBlackboard &blackboard);
    virtual ~AIParallelNode();

    void SetParallelParams(size_t x_as_passed, size_t x_as_finished);

    virtual Status Run();

protected:
    virtual bool InternalEvaluate();

    virtual void OnInterrupt();
    virtual void OnFinish();

private:
    size_t x_as_passed_, x_as_finished_;
    std::vector<size_t> current_selected_index_;
};
