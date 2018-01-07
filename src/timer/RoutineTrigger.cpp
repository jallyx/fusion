#include "RoutineTrigger.h"
#include "System.h"

time_t GetTriggerInterval(TriggerCycle tc)
{
    switch (tc) {
    case ByWeeklyTrigger:
        return 60*60*24*7;
    case ByDailyTrigger:
        return 60*60*24;
    case ByHourlyTrigger:
        return 60*60;
    case ByMinutelyTrigger:
        return 60;
    }
    return 0;
}

time_t GetTriggerPointTime(TriggerCycle tc, TriggerPoint tp)
{
    time_t point_time = GET_UNIX_TIME;
    const struct tm tm = GET_DATE_TIME;
    switch (tc) {
    case ByWeeklyTrigger:
        point_time += (tp.wday - tm.tm_wday) * (60*60*24);
    case ByDailyTrigger:
        point_time += (tp.hour - tm.tm_hour) * (60*60);
    case ByHourlyTrigger:
        point_time += (tp.min - tm.tm_min) * (60);
    case ByMinutelyTrigger:
        point_time += (tp.sec - tm.tm_sec) * (1);
    }
    return point_time;
}

time_t CalcPreviousTriggerPointTime(TriggerCycle tc, TriggerPoint tp)
{
    const time_t current_time = GET_UNIX_TIME;
    time_t trigger_interval = GetTriggerInterval(tc);
    time_t trigger_point_time = GetTriggerPointTime(tc, tp);
    if (trigger_point_time > current_time)
        trigger_point_time -= trigger_interval;
    return trigger_point_time;
}

time_t CalcNextTriggerPointTime(TriggerCycle tc, TriggerPoint tp)
{
    const time_t current_time = GET_UNIX_TIME;
    time_t trigger_interval = GetTriggerInterval(tc);
    time_t trigger_point_time = GetTriggerPointTime(tc, tp);
    if (trigger_point_time <= current_time)
        trigger_point_time += trigger_interval;
    return trigger_point_time;
}
