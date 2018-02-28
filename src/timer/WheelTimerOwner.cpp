#include "WheelTimerOwner.h"
#include "WheelTimer.h"
#include "WheelTimerMgr.h"

class WheelTimerOwner::Timer : public WheelTimer
{
public:
    Timer(WheelTimerOwner *owner, const std::function<void()> &cb,
          uint32 type, uint64 interval, uint32 repeats)
        : WheelTimer(interval, repeats)
        , cb_(cb)
        , type_(type)
        , owner_(owner->linked_from_this())
    {
    }

    virtual ~Timer()
    {
        DetachOwner();
    }

    void DetachOwner()
    {
        if (!owner_.expired()) {
            owner_.lock()->timers_.erase(itr_);
            owner_.reset();
        }
    }

    uint32 GetRoutineType() const { return type_; }

protected:
    virtual bool OnPrepare()
    {
        if (owner_.expired()) return false;
        itr_ = owner_.lock()->timers_.emplace(type_, this);
        WheelTimer::OnPrepare();
        return true;
    }

    virtual void OnActivate()
    {
        cb_();
    }

    virtual void RemoveRelevance()
    {
        owner_.reset();
        WheelTimer::RemoveRelevance();
    }

private:
    const std::function<void()> cb_;
    const uint32 type_;

    std::multimap<uint32, Timer*>::iterator itr_;
    std::weak_ptr<WheelTimerOwner> owner_;
};


WheelTimerOwner::WheelTimerOwner()
{
}

WheelTimerOwner::~WheelTimerOwner()
{
    RemoveTimers();
}

void WheelTimerOwner::CreateTimer(const std::function<void()> &cb,
    uint32 type, uint64 interval, uint32 repeats, uint64 firstime)
{
    GetCacheWheelTimerMgr()->Push(
        new Timer(this, cb, type, interval, repeats), firstime);
}

void WheelTimerOwner::CreateTimerX(const std::function<void()> &cb,
    uint64 interval, uint32 repeats, uint64 firstime)
{
    GetCacheWheelTimerMgr()->Push(
        new Timer(this, cb, ~0, interval, repeats), firstime);
}

void WheelTimerOwner::RemoveTimers(uint32 type) {
    RemoveRoutines(timers_, type);
}
void WheelTimerOwner::RemoveTimers() {
    RemoveRoutines(timers_);
}
bool WheelTimerOwner::HasTimer(uint32 type) const {
    return HasRoutine(timers_, type);
}
bool WheelTimerOwner::HasTimer() const {
    return HasRoutine(timers_);
}
template <>
WheelTimer *WheelTimerOwner::FindTimer(uint32 type) const {
    return FindRoutine(timers_, type);
}
