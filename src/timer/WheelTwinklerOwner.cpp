#include "WheelTwinklerOwner.h"
#include "WheelTwinkler.h"
#include "WheelTimerMgr.h"
#include "System.h"
#include "Logger.h"

class WheelTwinklerOwner::Twinkler : public WheelTwinkler
{
public:
    Twinkler(WheelTwinklerOwner *owner,
             TriggerCycle trigger_cycle, TriggerPoint trigger_point, time_t trigger_duration,
             const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
             uint32 routine_type, bool is_isolate, bool is_restore, uint32 repeats)
        : WheelTwinkler(
        trigger_cycle, trigger_point, trigger_duration,
        is_isolate, is_restore, repeats)
        , start_cb_(start_cb)
        , stop_cb_(stop_cb)
        , routine_type_(routine_type)
        , owner_(owner->linked_from_this())
    {
    }

    Twinkler(WheelTwinklerOwner *owner,
             uint64 trigger_interval, uint64 trigger_point, uint64 trigger_duration,
             const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
             uint32 routine_type, bool is_strict, bool is_isolate, bool is_restore,
             uint32 repeats)
        : WheelTwinkler(
        trigger_interval, trigger_point, trigger_duration,
        is_strict, is_isolate, is_restore, repeats)
        , start_cb_(start_cb)
        , stop_cb_(stop_cb)
        , routine_type_(routine_type)
        , owner_(owner->linked_from_this())
    {
    }

    virtual ~Twinkler()
    {
        DetachOwner();
    }

    void DetachOwner()
    {
        if (!owner_.expired()) {
            owner_.lock()->twinklers_.erase(itr_);
            owner_.reset();
        }
    }

    uint32 GetRoutineType() const { return routine_type_; }

protected:
    virtual bool OnPrepare()
    {
        if (owner_.expired()) return false;
        itr_ = owner_.lock()->twinklers_.emplace(routine_type_, this);
        return WheelTwinkler::OnPrepare();
    }

    virtual void OnStartActive()
    {
        start_cb_();
    }

    virtual void OnStopActive()
    {
        stop_cb_();
    }

    virtual void RemoveRelevance()
    {
        owner_.reset();
        WheelTwinkler::RemoveRelevance();
    }

private:
    const std::function<void()> start_cb_;
    const std::function<void()> stop_cb_;
    const uint32 routine_type_;

    std::multimap<uint32, Twinkler*>::iterator itr_;
    std::weak_ptr<WheelTwinklerOwner> owner_;
};

class WheelTwinklerOwner::TwinklerByMonthly : public WheelTwinklerOwner::Twinkler {
public:
    TwinklerByMonthly(WheelTwinklerOwner *owner, TriggerPoint trigger_point, time_t trigger_duration,
                      const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
                      uint32 routine_type, bool is_isolate, bool is_restore, uint32 repeats)
        : Twinkler(owner, MAX_CYCLE_MONTHLY, CalcPreviousTriggerPointTimeByMonthly(trigger_point)
        , trigger_duration, start_cb, stop_cb, routine_type, true, is_isolate, is_restore, repeats)
        , trigger_point_(trigger_point)
    {
    }

protected:
    virtual bool OnPrepare()
    {
        if (GetActualTickTime() > uint64(GET_UNIX_TIME)) {
            WLOG("CreateTwinklerByMonthly is invalid.");
            return false;
        }
        const uint64 init_point_time = point_time();
        if (!Twinkler::OnPrepare()) {
            return false;
        }
        if (point_time() != init_point_time) {
            set_point_time(CalcNextTriggerPointTimeByMonthly(trigger_point_));
            SetNextActiveTime(point_time());
        }
        return true;
    }

    virtual void OnStopActive()
    {
        set_point_time(CalcNextTriggerPointTimeByMonthly(trigger_point_));
        SetNextActiveTime(point_time());
        Twinkler::OnStopActive();
    }

private:
    const TriggerPoint trigger_point_;
};


WheelTwinklerOwner::WheelTwinklerOwner()
{
}

WheelTwinklerOwner::~WheelTwinklerOwner()
{
    RemoveTwinklers();
}

void WheelTwinklerOwner::CreateTwinkler(
    TriggerCycle trigger_cycle, TriggerPoint trigger_point, time_t trigger_duration,
    const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
    uint32 routine_type, bool is_isolate, bool is_restore, uint32 repeats)
{
    switch (trigger_cycle) {
    case ByMonthlyTrigger:
        GetCacheWheelTimerMgr()->Push(new TwinklerByMonthly(
            this, trigger_point, trigger_duration,
            start_cb, stop_cb, routine_type, is_isolate, is_restore, repeats));
        break;
    default:
        GetCacheWheelTimerMgr()->Push(new Twinkler(
            this, trigger_cycle, trigger_point, trigger_duration,
            start_cb, stop_cb, routine_type, is_isolate, is_restore, repeats));
        break;
    }
}

void WheelTwinklerOwner::CreateTwinklerX(
    TriggerCycle trigger_cycle, TriggerPoint trigger_point, time_t trigger_duration,
    const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
    bool is_isolate, bool is_restore, uint32 repeats)
{
    switch (trigger_cycle) {
    case ByMonthlyTrigger:
        GetCacheWheelTimerMgr()->Push(new TwinklerByMonthly(
            this, trigger_point, trigger_duration,
            start_cb, stop_cb, ~0, is_isolate, is_restore, repeats));
        break;
    default:
        GetCacheWheelTimerMgr()->Push(new Twinkler(
            this, trigger_cycle, trigger_point, trigger_duration,
            start_cb, stop_cb, ~0, is_isolate, is_restore, repeats));
        break;
    }
}

void WheelTwinklerOwner::CreateTwinkler(
    uint64 trigger_interval, uint64 trigger_point, uint64 trigger_duration,
    const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
    uint32 routine_type, bool is_strict, bool is_isolate, bool is_restore, uint32 repeats)
{
    GetCacheWheelTimerMgr()->Push(new Twinkler(
        this, trigger_interval, trigger_point, trigger_duration,
        start_cb, stop_cb, routine_type, is_strict, is_isolate, is_restore, repeats));
}

void WheelTwinklerOwner::CreateTwinklerX(
    uint64 trigger_interval, uint64 trigger_point, uint64 trigger_duration,
    const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
    bool is_strict, bool is_isolate, bool is_restore, uint32 repeats)
{
    GetCacheWheelTimerMgr()->Push(new Twinkler(
        this, trigger_interval, trigger_point, trigger_duration,
        start_cb, stop_cb, ~0, is_strict, is_isolate, is_restore, repeats));
}

void WheelTwinklerOwner::RemoveTwinklers(uint32 type) {
    RemoveRoutines(twinklers_, type);
}
void WheelTwinklerOwner::RemoveTwinklers() {
    RemoveRoutines(twinklers_);
}
bool WheelTwinklerOwner::HasTwinkler(uint32 type) const {
    return HasRoutine(twinklers_, type);
}
bool WheelTwinklerOwner::HasTwinkler() const {
    return HasRoutine(twinklers_);
}
template <>
WheelTwinkler *WheelTwinklerOwner::FindTwinkler(uint32 type) const {
    return FindRoutine(twinklers_, type);
}
