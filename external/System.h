#pragma once

#include <time.h>
#include <random>
#include "Base.h"
#include "Macro.h"

class System
{
public:
    static void Init();
    static void Update();

    static int Rand(int lower, int upper);
    static float Randf(float lower, float upper);

    static time_t GetMinuteUnixTime();
    static time_t GetDayUnixTime();
    static time_t GetWeekUnixTime();
    static time_t GetMonthUnixTime();

    static uint64 GetRealSysTime();

    static time_t GetUnixTime() { return unix_time_; }
    static uint64 GetSysTime() { return sys_time_; }
    static uint64 GetStartTime() { return start_time_; }
    static struct tm GetDateTime() { return date_time_; }

private:
    static time_t unix_time_;
    static uint64 sys_time_;
    static uint64 start_time_;
    static struct tm date_time_;
#if defined(_WIN32)
    static LARGE_INTEGER performance_frequency_;
#endif
    static std::default_random_engine random_engine_;
};

#define GET_UNIX_TIME (System::GetUnixTime())
#define GET_SYS_TIME (System::GetSysTime())
#define GET_DATE_TIME (System::GetDateTime())

#define GET_MINUTE_UNIX_TIME (System::GetMinuteUnixTime())
#define GET_DAY_UNIX_TIME (System::GetDayUnixTime())
#define GET_WEEK_UNIX_TIME (System::GetWeekUnixTime())
#define GET_MONTH_UNIX_TIME (System::GetMonthUnixTime())

#define GET_REAL_SYS_TIME (System::GetRealSysTime())

#define GET_APP_TIME (System::GetSysTime() - System::GetStartTime())
#define GET_REAL_APP_TIME (System::GetRealSysTime() - System::GetStartTime())
