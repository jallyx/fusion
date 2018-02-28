#pragma once

#include "WheelRoutineOwner.h"
#include "RoutineTrigger.h"
#include <functional>
#include "enable_linked_from_this.h"

class WheelTriggerOwner : virtual protected WheelRoutineOwner,
    public enable_linked_from_this<WheelTriggerOwner>
{
public:
    WheelTriggerOwner();
    ~WheelTriggerOwner();

    void CreateTrigger(TriggerCycle trigger_cycle, TriggerPoint trigger_point,
        const std::function<void()> &cb, uint32 type, uint32 repeats = 0);
    void CreateTriggerX(TriggerCycle trigger_cycle, TriggerPoint trigger_point,
        const std::function<void()> &cb, uint32 repeats = 0);

    void CreateTrigger(time_t trigger_interval, time_t trigger_point,
        const std::function<void()> &cb, uint32 type, uint32 repeats = 0);
    void CreateTriggerX(time_t trigger_interval, time_t trigger_point,
        const std::function<void()> &cb, uint32 repeats = 0);

    void RemoveTriggers(uint32 type);
    void RemoveTriggers();
    bool HasTrigger(uint32 type) const;
    bool HasTrigger() const;
    template <typename T> T *FindTrigger(uint32 type) const;

private:
    class Trigger;
    class TriggerByMonthly;
    std::multimap<uint32, Trigger*> triggers_;
};
