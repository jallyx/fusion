#pragma once

#include "WheelTrigger.h"

class WheelTwinkler : public WheelTrigger
{
public:
    WheelTwinkler(TriggerCycle trigger_cycle,
        TriggerPoint trigger_point, time_t trigger_duration,
        bool is_isolate = false, bool is_restore = false, uint32 loop_count = 0);
    WheelTwinkler(uint64 trigger_interval,
        uint64 trigger_point, uint64 trigger_duration, bool is_strict = true,
        bool is_isolate = false, bool is_restore = false, uint32 loop_count = 0);

protected:
    virtual ~WheelTwinkler();
    virtual bool OnPrepare();
    virtual void OnActivate();

    virtual void OnStartActive() {}
    virtual void OnStopActive() {}

    uint64 trigger_duration() const { return trigger_duration_; }

private:
    uint64 GetNextActivateTime() const {
        return !is_start_ ? point_time() : point_time() + trigger_duration();
    }

    const uint64 trigger_duration_;
    const bool is_strict_;
    const bool is_isolate_;
    const bool is_restore_;
    bool is_start_;
};
