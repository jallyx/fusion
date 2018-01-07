#pragma once

#include "WheelTrigger.h"

class WheelTwinkler : public WheelTrigger
{
public:
    WheelTwinkler(TriggerCycle trigger_cycle,
        TriggerPoint trigger_point, time_t trigger_duration,
        bool is_isolate = false, bool is_restore = false, uint32 loop_count = 0);
    WheelTwinkler(time_t trigger_interval,
        time_t trigger_point, time_t trigger_duration, bool is_strict = true,
        bool is_isolate = false, bool is_restore = false, uint32 loop_count = 0);

protected:
    virtual ~WheelTwinkler();
    virtual bool OnPrepare();
    virtual void OnActivate();

    virtual void OnStartActive() = 0;
    virtual void OnStopActive() = 0;

    time_t trigger_duration() const { return trigger_duration_; }

private:
    time_t GetNextActivateTime() const {
        return !is_start_ ? point_time() : point_time() + trigger_duration();
    }

    const time_t trigger_duration_;
    const bool is_strict_;
    const bool is_isolate_;
    const bool is_restore_;
    bool is_start_;
};
