#pragma once

#include <time.h>
#include "Base.h"

enum TriggerCycle
{
    ByWeeklyTrigger,
    ByDailyTrigger,
    ByHourlyTrigger,
    ByMinutelyTrigger,
};

struct TriggerPoint
{
    uint8 wday;
    uint8 hour;
    uint8 min;
    uint8 sec;
};

inline TriggerPoint MakeTriggerPoint(uint8 sec)
{ TriggerPoint tp = {0,0,0,sec}; return tp; }
inline TriggerPoint MakeTriggerPoint(uint8 min, uint8 sec)
{ TriggerPoint tp = {0,0,min,sec}; return tp; }
inline TriggerPoint MakeTriggerPoint(uint8 hour, uint8 min, uint8 sec)
{ TriggerPoint tp = {0,hour,min,sec}; return tp; }
inline TriggerPoint MakeTriggerPoint(uint8 wday, uint8 hour, uint8 min, uint8 sec)
{ TriggerPoint tp = {wday,hour,min,sec}; return tp; }

time_t GetTriggerInterval(TriggerCycle tc);
time_t GetTriggerPointTime(TriggerCycle tc, TriggerPoint tp);

time_t CalcPreviousTriggerPointTime(TriggerCycle tc, TriggerPoint tp);
time_t CalcNextTriggerPointTime(TriggerCycle tc, TriggerPoint tp);
