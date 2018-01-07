#include "WheelTriggerOwner.h"
#include "WheelTrigger.h"
#include "WheelTimerMgr.h"

class WheelTriggerOwner::Trigger : public WheelTrigger
{
public:
    Trigger(WheelTriggerOwner *owner,
            TriggerCycle trigger_cycle, TriggerPoint trigger_point,
            const std::function<void()> &cb, uint32 type, uint32 repeats)
        : WheelTrigger(trigger_cycle, trigger_point, repeats)
        , cb_(cb)
        , type_(type)
        , owner_(owner->linked_from_this())
    {
    }

    Trigger(WheelTriggerOwner *owner,
            time_t trigger_interval, time_t trigger_point,
            const std::function<void()> &cb, uint32 type, uint32 repeats)
        : WheelTrigger(trigger_interval, trigger_point, repeats)
        , cb_(cb)
        , type_(type)
        , owner_(owner->linked_from_this())
    {
    }

    virtual ~Trigger()
    {
        DetachOwner();
    }

    void DetachOwner()
    {
        if (!owner_.expired()) {
            owner_.lock()->trigger_list_.erase(itr_);
            owner_.reset();
        }
    }

    uint32 GetRoutineType() const { return type_; }

protected:
    virtual bool OnPrepare()
    {
        if (owner_.expired()) return false;
        std::list<Trigger*> &tlist = owner_.lock()->trigger_list_;
        itr_ = tlist.insert(tlist.end(), this);
        WheelTrigger::OnPrepare();
        return true;
    }

    virtual void OnActivate()
    {
        cb_();
    }

    virtual void RemoveRelevance()
    {
        owner_.reset();
        WheelTrigger::RemoveRelevance();
    }

private:
    const std::function<void()> cb_;
    const uint32 type_;

    std::list<Trigger*>::iterator itr_;
    std::weak_ptr<WheelTriggerOwner> owner_;
};


WheelTriggerOwner::WheelTriggerOwner()
{
}

WheelTriggerOwner::~WheelTriggerOwner()
{
    RemoveTriggers();
}

void WheelTriggerOwner::CreateTrigger(
    TriggerCycle trigger_cycle, TriggerPoint trigger_point,
    const std::function<void()> &cb, uint32 type, uint32 repeats)
{
    GetCacheWheelTimerMgr()->Push(new Trigger(
        this, trigger_cycle, trigger_point, cb, type, repeats));
}

void WheelTriggerOwner::CreateTriggerX(
    TriggerCycle trigger_cycle, TriggerPoint trigger_point,
    const std::function<void()> &cb, uint32 repeats)
{
    GetCacheWheelTimerMgr()->Push(new Trigger(
        this, trigger_cycle, trigger_point, cb, ~0, repeats));
}

void WheelTriggerOwner::CreateTrigger(
    time_t trigger_interval, time_t trigger_point,
    const std::function<void()> &cb, uint32 type, uint32 repeats)
{
    GetCacheWheelTimerMgr()->Push(new Trigger(
        this, trigger_interval, trigger_point, cb, type, repeats));
}

void WheelTriggerOwner::CreateTriggerX(
    time_t trigger_interval, time_t trigger_point,
    const std::function<void()> &cb, uint32 repeats)
{
    GetCacheWheelTimerMgr()->Push(new Trigger(
        this, trigger_interval, trigger_point, cb, ~0, repeats));
}

void WheelTriggerOwner::RemoveTriggers(uint32 type) {
    RemoveRoutines(trigger_list_, type);
}
void WheelTriggerOwner::RemoveTriggers() {
    RemoveRoutines(trigger_list_);
}
bool WheelTriggerOwner::HasTrigger(uint32 type) const {
    return HasRoutine(trigger_list_, type);
}
bool WheelTriggerOwner::HasTrigger() const {
    return HasRoutine(trigger_list_);
}
template <>
WheelTrigger *WheelTriggerOwner::FindTrigger(uint32 type) const {
    return FindRoutine(trigger_list_, type);
}
