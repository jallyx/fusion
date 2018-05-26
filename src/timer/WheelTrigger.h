#pragma once

#include "WheelTimer.h"
#include "RoutineTrigger.h"

class WheelTrigger : public WheelTimer
{
public:
    WheelTrigger(TriggerCycle trigger_cycle,
        TriggerPoint trigger_point, uint32 loop_count = 0);
    WheelTrigger(time_t trigger_interval,
        time_t trigger_point, uint32 loop_count = 0);

protected:
    virtual ~WheelTrigger();
    virtual bool OnPrepare();
    virtual void OnActivate();

    virtual void OnActive() {}

    void GoNextActiveTime();

    void set_point_time(time_t t) { point_time_ = t; }
    void go_previous_point_time() { point_time_ -= active_interval(); }
    void go_next_point_time() { point_time_ += active_interval(); }
    time_t point_time() const { return point_time_; }

private:
    time_t point_time_;
};
