#include "System.h"
#include <tuple>
#include <utility>

time_t System::unix_time_;
uint64 System::sys_time_;
uint64 System::start_time_;
struct tm System::date_time_;
#if defined(_WIN32)
    LARGE_INTEGER System::performance_frequency_;
#endif
std::default_random_engine System::random_engine_;

void System::Init()
{
#if defined(_WIN32)
    QueryPerformanceFrequency(&performance_frequency_);
#endif
    random_engine_.seed(std::random_device()());
    start_time_ = GetRealSysTime();
}

void System::Update()
{
    auto atomic_time = []()->std::pair<time_t, struct tm> {
        std::pair<time_t, struct tm> result;
        time(&result.first);
        localtime_r(&result.first, &result.second);
        return result;
    };
    std::tie(unix_time_, date_time_) = atomic_time();
    sys_time_ = GetRealSysTime();
}

int System::Rand(int lower, int upper)
{
    if (lower > upper) std::swap(lower, upper);
    if (lower == upper || lower == upper - 1) return lower;
    return std::uniform_int_distribution<int>(lower, upper - 1)(random_engine_);
}

float System::Randf(float lower, float upper)
{
    if (lower == upper) return lower;
    if (lower > upper) std::swap(lower, upper);
    return std::uniform_real_distribution<float>(lower, upper)(random_engine_);
}

static inline int PassDayTime(const struct tm &tm) {
    return tm.tm_hour * 60*60 + tm.tm_min * 60 + tm.tm_sec;
}
static inline int PassWeekTime(const struct tm &tm) {
    return (tm.tm_wday != 0 ? tm.tm_wday - 1 : 6) * 60*60*24 + PassDayTime(tm);
}
static inline int PassMonthTime(const struct tm &tm) {
    return (tm.tm_mday - 1) * 60*60*24 + PassDayTime(tm);
}

time_t System::GetDayUnixTime()
{
    return unix_time_ - PassDayTime(date_time_);
}

time_t System::GetWeekUnixTime()
{
    return unix_time_ - PassWeekTime(date_time_);
}

time_t System::GetMonthUnixTime()
{
    return unix_time_ - PassMonthTime(date_time_);
}

uint64 System::GetRealSysTime()
{
#if defined(_WIN32)
    LARGE_INTEGER performance_counter;
    QueryPerformanceCounter(&performance_counter);
    return 1000 * performance_counter.QuadPart / performance_frequency_.QuadPart;
#else
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
#endif
}
