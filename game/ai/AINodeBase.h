#pragma once

#include "lua/LuaRef.h"

class AIBlackboard;

class AINodeBase
{
public:
    AINodeBase(AIBlackboard &blackboard);
    virtual ~AINodeBase();

    int SetExternalPrecondition(lua_State *L);

    enum PathValue {
        Unreachable,
        Reachable,
        Runnable,
    };
    PathValue Evaluate();

    enum Status {
        Idle,
        Running,
        Finished,
    };
    virtual Status Run() = 0;

    void Interrupt();
    void Finish();

    Status status() const { return status_; }

protected:
    virtual bool InternalEvaluate() { return true; }

    virtual void OnInterrupt() {}
    virtual void OnFinish() {}

    AIBlackboard &blackboard_;
    Status status_;

private:
    bool ExternalEvaluate() const;

    LuaRef external_precondition_;
};
