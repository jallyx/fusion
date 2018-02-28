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
