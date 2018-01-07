#include "RoutineExtra.h"
#include "System.h"
#include "Logger.h"

#define MAX_CYCLE_MONTHLY (60*60*24*31)

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

WheelTrigger *CreateTriggerByMonthly(
    TriggerPoint trigger_point, const std::function<void()> &cb, uint32 repeats)
{
    class Trigger : public WheelTrigger {
    public:
        Trigger(TriggerPoint trigger_point, const std::function<void()> &cb, uint32 repeats)
            : WheelTrigger(
            MAX_CYCLE_MONTHLY, CalcNextTriggerPointTimeByMonthly(trigger_point), repeats)
            , trigger_point_(trigger_point)
            , cb_(cb)
        {}
    protected:
        virtual bool OnPrepare() {
            WheelTrigger::OnPrepare();
            if (GetCurrentTickTime() > (uint64)GET_UNIX_TIME) {
                WLOG("CreateTriggerByMonthly is invalid.");
                return false;
            }
            return true;
        }
        virtual void OnActivate() {
            set_point_time(CalcNextTriggerPointTimeByMonthly(trigger_point_));
            SetNextActiveTime(point_time());
            cb_();
        }
    private:
        const TriggerPoint trigger_point_;
        const std::function<void()> cb_;
    };
    return new Trigger(trigger_point, cb, repeats);
}

WheelTwinkler *CreateTwinklerByMonthly(
    TriggerPoint trigger_point, time_t trigger_duration,
    const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
    bool is_isolate, bool is_restore, uint32 repeats)
{
    class Twinkler : public WheelTwinkler {
    public:
        Twinkler(TriggerPoint trigger_point, time_t trigger_duration,
                 const std::function<void()> &start_cb, const std::function<void()> &stop_cb,
                 bool is_isolate, bool is_restore, uint32 repeats)
            : WheelTwinkler(
            MAX_CYCLE_MONTHLY, CalcPreviousTriggerPointTimeByMonthly(trigger_point),
            trigger_duration, true, is_isolate, is_restore, repeats)
            , trigger_point_(trigger_point)
            , start_cb_(start_cb)
            , stop_cb_(stop_cb)
        {}
    protected:
        virtual bool OnPrepare() {
            const time_t previous_point_time = point_time();
            WheelTwinkler::OnPrepare();
            if (GetCurrentTickTime() <= (uint64)GET_UNIX_TIME) {
                if (point_time() != previous_point_time) {
                    set_point_time(CalcNextTriggerPointTimeByMonthly(trigger_point_));
                    SetNextActiveTime(point_time());
                }
            } else {
                WLOG("CreateTwinklerByMonthly is invalid.");
                return false;
            }
            return true;
        }
        virtual void OnStartActive() {
            start_cb_();
        }
        virtual void OnStopActive() {
            set_point_time(CalcNextTriggerPointTimeByMonthly(trigger_point_));
            SetNextActiveTime(point_time());
            stop_cb_();
        }
    private:
        const TriggerPoint trigger_point_;
        const std::function<void()> start_cb_;
        const std::function<void()> stop_cb_;
    };
    return new Twinkler(
        trigger_point, trigger_duration, start_cb, stop_cb, is_isolate, is_restore, repeats);
}
