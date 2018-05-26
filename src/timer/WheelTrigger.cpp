#include "WheelTrigger.h"

WheelTrigger::WheelTrigger(
    TriggerCycle trigger_cycle, TriggerPoint trigger_point, uint32 loop_count)
: WheelTrigger(GetTriggerInterval(trigger_cycle)
, CalcPreviousTriggerPointTime(trigger_cycle, trigger_point)
, loop_count)
{
}

WheelTrigger::WheelTrigger(
    time_t trigger_interval, time_t trigger_point, uint32 loop_count)
: WheelTimer(trigger_interval, loop_count)
, point_time_(trigger_point)
{
}

WheelTrigger::~WheelTrigger()
{
}

void WheelTrigger::GoNextActiveTime()
{
    const time_t actual_tick_time = GetActualTickTime();
    while (actual_tick_time > point_time()) {
        go_next_point_time();
    }
}

bool WheelTrigger::OnPrepare()
{
    GoNextActiveTime();
    SetNextActiveTime(point_time());
    return true;
}

void WheelTrigger::OnActivate()
{
    GoNextActiveTime();
    SetNextActiveTime(point_time());
    OnActive();
}
