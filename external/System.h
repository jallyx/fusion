#pragma once

#include <time.h>
#include "Base.h"
#include "Macro.h"

class System
{
public:
    static void Init();
    static void Update();

    static int Rand(int lower, int upper);
    static float Randf(float lower, float upper);

    static time_t GetDayUnixTime();
    static time_t GetWeekUnixTime();

    static uint64 GetRealSysTime();

    static time_t GetUnixTime() { return unix_time_; }
    static uint64 GetSysTime() { return sys_time_; }
    static struct tm GetDateTime() { return date_time_; }

private:
    static time_t unix_time_;
    static uint64 sys_time_;
    static struct tm date_time_;
#if defined(_WIN32)
    static LARGE_INTEGER performance_frequency_;
#endif
};

#define GET_UNIX_TIME (System::GetUnixTime())
#define GET_SYS_TIME (System::GetSysTime())
#define GET_DATE_TIME (System::GetDateTime())

#define GET_DAY_UNIX_TIME (System::GetDayUnixTime())
#define GET_WEEK_UNIX_TIME (System::GetWeekUnixTime())

#define GET_REAL_SYS_TIME (System::GetRealSysTime())
