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
            owner_.lock()->timer_list_.erase(itr_);
            owner_.reset();
        }
    }

    uint32 GetRoutineType() const { return type_; }

protected:
    virtual bool OnPrepare()
    {
        if (owner_.expired()) return false;
        std::list<Timer*> &tlist = owner_.lock()->timer_list_;
        itr_ = tlist.insert(tlist.end(), this);
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

    std::list<Timer*>::iterator itr_;
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
    RemoveRoutines(timer_list_, type);
}
void WheelTimerOwner::RemoveTimers() {
    RemoveRoutines(timer_list_);
}
bool WheelTimerOwner::HasTimer(uint32 type) const {
    return HasRoutine(timer_list_, type);
}
bool WheelTimerOwner::HasTimer() const {
    return HasRoutine(timer_list_);
}
template <>
WheelTimer *WheelTimerOwner::FindTimer(uint32 type) const {
    return FindRoutine(timer_list_, type);
}
