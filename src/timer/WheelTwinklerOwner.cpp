#include "WheelTwinklerOwner.h"
#include "WheelTwinkler.h"
#include "WheelTimerMgr.h"

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
             time_t trigger_interval, time_t trigger_point, time_t trigger_duration,
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
            owner_.lock()->twinkler_list_.erase(itr_);
            owner_.reset();
        }
    }

    uint32 GetRoutineType() const { return routine_type_; }

protected:
    virtual bool OnPrepare()
    {
        if (owner_.expired()) return false;
        std::list<Twinkler*> &tlist = owner_.lock()->twinkler_list_;
        itr_ = tlist.insert(tlist.end(), this);
        WheelTwinkler::OnPrepare();
        return true;
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

    std::list<Twinkler*>::iterator itr_;
    std::weak_ptr<WheelTwinklerOwner> owner_;
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
    GetCacheWheelTimerMgr()->Push(new Twinkler(
        this, trigger_cycle, trigger_point, trigger_duration,
        start_cb, stop_cb, routine_type, is_isolate, is_restore, repeats));
}

void WheelTwinklerOwner::CreateTwinklerX(
    TriggerCycle trigger_cycle, TriggerPoint trigger_point, time_t trigger_duration,
    const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
    bool is_isolate, bool is_restore, uint32 repeats)
{
    GetCacheWheelTimerMgr()->Push(new Twinkler(
        this, trigger_cycle, trigger_point, trigger_duration,
        start_cb, stop_cb, ~0, is_isolate, is_restore, repeats));
}

void WheelTwinklerOwner::CreateTwinkler(
    time_t trigger_interval, time_t trigger_point, time_t trigger_duration,
    const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
    uint32 routine_type, bool is_strict, bool is_isolate, bool is_restore, uint32 repeats)
{
    GetCacheWheelTimerMgr()->Push(new Twinkler(
        this, trigger_interval, trigger_point, trigger_duration,
        start_cb, stop_cb, routine_type, is_strict, is_isolate, is_restore, repeats));
}

void WheelTwinklerOwner::CreateTwinklerX(
    time_t trigger_interval, time_t trigger_point, time_t trigger_duration,
    const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
    bool is_strict, bool is_isolate, bool is_restore, uint32 repeats)
{
    GetCacheWheelTimerMgr()->Push(new Twinkler(
        this, trigger_interval, trigger_point, trigger_duration,
        start_cb, stop_cb, ~0, is_strict, is_isolate, is_restore, repeats));
}

void WheelTwinklerOwner::RemoveTwinklers(uint32 type) {
    RemoveRoutines(twinkler_list_, type);
}
void WheelTwinklerOwner::RemoveTwinklers() {
    RemoveRoutines(twinkler_list_);
}
bool WheelTwinklerOwner::HasTwinkler(uint32 type) const {
    return HasRoutine(twinkler_list_, type);
}
bool WheelTwinklerOwner::HasTwinkler() const {
    return HasRoutine(twinkler_list_);
}
template <>
WheelTwinkler *WheelTwinklerOwner::FindTwinkler(uint32 type) const {
    return FindRoutine(twinkler_list_, type);
}
