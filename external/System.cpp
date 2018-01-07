#include "System.h"
#include <stdint.h>
#include <stdlib.h>
#include <algorithm>
#include <tuple>
#include "Macro.h"

namespace {
struct SystemLoader {
    SystemLoader() {
        System::Init();
        System::Update();
    }
}_;
}

time_t System::unix_time_;
uint64 System::sys_time_;
struct tm System::date_time_;
#if defined(_WIN32)
    LARGE_INTEGER System::performance_frequency_;
#endif

void System::Init()
{
    srand((unsigned)time(nullptr));
#if defined(_WIN32)
    QueryPerformanceFrequency(&performance_frequency_);
#endif
}

void System::Update()
{
    auto atom = []()->std::pair<time_t, struct tm> {
        std::pair<time_t, struct tm> result;
        time(&result.first);
        localtime_r(&result.first, &result.second);
        return result;
    };
    std::tie(unix_time_, date_time_) = atom();
    sys_time_ = GetRealSysTime();
}

static inline uint32_t SystemRandomU32() {
    return uint32_t((rand() << 22) ^ (rand() << 11) ^ rand());
}
static inline float SystemRandomF32() {
    return float((double)SystemRandomU32() / UINT32_MAX);
}

int System::Rand(int lower, int upper)
{
    if (lower == upper) return lower;
    if (lower > upper) std::swap(lower, upper);
    return lower + SystemRandomU32() % (upper - lower);
}

float System::Randf(float lower, float upper)
{
    if (lower == upper) return lower;
    if (lower > upper) std::swap(lower, upper);
    return lower + (upper - lower) * SystemRandomF32();
}

static inline int PassDayTime(const struct tm &tm) {
    return tm.tm_hour * 60*60 + tm.tm_min * 60 + tm.tm_sec;
}
static inline int PassWeekTime(const struct tm &tm) {
    return (tm.tm_wday != 0 ? tm.tm_wday - 1 : 6) * 60*60*24 + PassDayTime(tm);
}

time_t System::GetDayUnixTime()
{
    return unix_time_ - PassDayTime(date_time_);
}

time_t System::GetWeekUnixTime()
{
    return unix_time_ - PassWeekTime(date_time_);
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
