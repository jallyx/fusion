#include "RoutineTrigger.h"
#include "System.h"

static inline bool IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static inline int GetDayOfMonth(int year, int month) {
    static int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    return month != 2 ? days[month - 1] : (IsLeapYear(year) ? 29 : 28);
}

static inline int DiffTimeByMonthly(const TriggerPoint &tp, const struct tm &tm) {
    return (tp.wday - tm.tm_mday) * (60*60*24) +
        (tp.hour - tm.tm_hour) * (60*60) +
        (tp.min - tm.tm_min) * (60) +
        (tp.sec - tm.tm_sec) * (1);
}

time_t CalcPreviousTriggerPointTimeByMonthly(TriggerPoint tp)
{
    time_t delay_time = 0;
    const time_t curtime = GET_UNIX_TIME;
    const struct tm tm = GET_DATE_TIME;
    if (tp.wday <= 31) {
        if ((delay_time = DiffTimeByMonthly(tp, tm)) > 0) {
            bool found = false;
            int month = tm.tm_mon - 1;
            for (int year = tm.tm_year; !found; --year, month = 11) {
                for (; month >= 0 && !found; --month) {
                    int days = GetDayOfMonth(year + 1900, month + 1);
                    delay_time -= days * 60*60*24;
                    if (days >= tp.wday)
                        found = true;
                }
            }
        }
    } else {
        tp.wday = GetDayOfMonth(tm.tm_year + 1900, tm.tm_mon + 1);
        if ((delay_time = DiffTimeByMonthly(tp, tm)) > 0) {
            delay_time -= tp.wday * 60*60*24;
        }
    }
    return curtime + delay_time;
}

time_t CalcNextTriggerPointTimeByMonthly(TriggerPoint tp)
{
    time_t delay_time = 0;
    const time_t curtime = GET_UNIX_TIME;
    const struct tm tm = GET_DATE_TIME;
    int days = GetDayOfMonth(tm.tm_year + 1900, tm.tm_mon + 1);
    if (tp.wday <= 31) {
        if ((delay_time = DiffTimeByMonthly(tp, tm)) <= 0 || days < tp.wday) {
            bool found = false;
            int month = tm.tm_mon + 1;
            for (int year = tm.tm_year; !found; ++year, month = 0) {
                for (; month < 12 && !found; ++month) {
                    delay_time += days * 60*60*24;
                    if ((days = GetDayOfMonth(year + 1900, month + 1)) >= tp.wday)
                        found = true;
                }
            }
        }
    } else {
        tp.wday = days;
        if ((delay_time = DiffTimeByMonthly(tp, tm)) <= 0) {
            days = GetDayOfMonth(
                (tm.tm_mon < 11 ? tm.tm_year : tm.tm_year + 1) + 1900,
                (tm.tm_mon < 11 ? tm.tm_mon + 1 : 0) + 1);
            delay_time += days * 60*60*24;
        }
    }
    return curtime + delay_time;
}

time_t GetTriggerInterval(TriggerCycle tc)
{
    switch (tc) {
    case ByMonthlyTrigger:
        return MAX_CYCLE_MONTHLY;
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
    case ByMonthlyTrigger:
        point_time += (tm.tm_wday - tm.tm_mday) * (60*60*24);
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
