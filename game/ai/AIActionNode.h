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
