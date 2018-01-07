#pragma once

#include "WheelTrigger.h"
#include "WheelTwinkler.h"
#include <functional>

time_t CalcPreviousTriggerPointTimeByMonthly(TriggerPoint tp);
time_t CalcNextTriggerPointTimeByMonthly(TriggerPoint tp);

WheelTrigger *CreateTriggerByMonthly(
    TriggerPoint trigger_point, const std::function<void()> &cb, uint32 repeats = 0);
WheelTwinkler *CreateTwinklerByMonthly(
    TriggerPoint trigger_point, time_t trigger_duration,
    const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
    bool is_isolate = false, bool is_restore = false, uint32 repeats = 0);
