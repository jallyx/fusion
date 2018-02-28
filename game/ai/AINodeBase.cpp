#include "AINodeBase.h"

AINodeBase::AINodeBase(AIBlackboard &blackboard)
: blackboard_(blackboard)
, status_(Idle)
{
}

AINodeBase::~AINodeBase()
{
}

int AINodeBase::SetExternalPrecondition(lua_State *L)
{
    external_precondition_ = LuaRef(L, 2);
    return 0;
}

AINodeBase::PathValue AINodeBase::Evaluate()
{
    if (ExternalEvaluate()) {
        if (InternalEvaluate())
            return Runnable;
        return Reachable;
    }
    if (status_ == Running)
        Interrupt();
    return Unreachable;
}

bool AINodeBase::ExternalEvaluate() const
{
    if (!external_precondition_.isref())
        return true;
    if (external_precondition_.Call<bool>())
        return true;
    return false;
}

void AINodeBase::Interrupt()
{
    if (status_ == Running) {
        status_ = Idle;
        OnInterrupt();
    }
}

void AINodeBase::Finish()
{
    if (status_ == Finished) {
        status_ = Idle;
        OnFinish();
    }
}
