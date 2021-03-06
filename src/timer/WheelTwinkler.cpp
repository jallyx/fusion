#include "WheelTwinkler.h"

WheelTwinkler::WheelTwinkler(
    TriggerCycle trigger_cycle, TriggerPoint trigger_point, time_t trigger_duration,
    bool is_isolate, bool is_restore, uint32 loop_count)
: WheelTrigger(trigger_cycle, trigger_point, loop_count)
, trigger_duration_(trigger_duration)
, is_strict_(true)
, is_isolate_(is_isolate)
, is_restore_(is_restore)
, is_start_(false)
{
}

WheelTwinkler::WheelTwinkler(
    uint64 trigger_interval, uint64 trigger_point, uint64 trigger_duration,
    bool is_strict, bool is_isolate, bool is_restore, uint32 loop_count)
: WheelTrigger(trigger_interval, trigger_point, loop_count)
, trigger_duration_(trigger_duration)
, is_strict_(is_strict)
, is_isolate_(is_isolate)
, is_restore_(is_restore)
, is_start_(false)
{
}

WheelTwinkler::~WheelTwinkler()
{
}

bool WheelTwinkler::OnPrepare()
{
    const uint64 actual_tick_time = GetActualTickTime();
    if (is_strict_) {
        while (true) {
            is_start_ = actual_tick_time > point_time();
            bool is_stop = actual_tick_time > point_time() + trigger_duration_;
            if ((is_start_ && is_stop) || (is_start_ && !is_stop && !is_isolate_)) {
                go_next_point_time();
                continue;
            }
            if (is_start_ && !is_stop && is_isolate_ && is_restore_) {
                is_start_ = false;
            }
            break;
        }
    } else {
        if (point_time() == 0) {
            set_point_time(actual_tick_time + active_interval());
        } else if (actual_tick_time > point_time()) {
            set_point_time(actual_tick_time);
        }
    }
    SetNextActiveTime(GetNextActivateTime());
    return true;
}

void WheelTwinkler::OnActivate()
{
    bool do_start = false, do_stop = false;
    const uint64 current_tick_time = GetCurrentTickTime();
    if (!is_start_ && current_tick_time >= point_time()) {
        do_start = is_start_ = true;
    }
    if (current_tick_time >= point_time() + trigger_duration_) {
        do_stop = true;
    }
    if (do_stop) {
        is_start_ = false;
        GoNextActiveTime();
    }
    if (do_start && !do_stop) {
        IgnoreOnceLoop();
    }
    SetNextActiveTime(GetNextActivateTime());

    if (do_start) {
        OnStartActive();
    }
    if (do_stop) {
        OnStopActive();
    }
}
