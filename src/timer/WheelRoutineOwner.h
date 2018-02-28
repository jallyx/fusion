#pragma once

#include <atomic>
#include <map>
#include "Base.h"
#include "noncopyable.h"

class WheelTimerMgr;

class WheelRoutineOwner : public noncopyable
{
public:
    WheelRoutineOwner() : timer_mgr_(nullptr) {}

protected:
    virtual WheelTimerMgr *GetWheelTimerMgr() = 0;

    inline WheelTimerMgr *GetCacheWheelTimerMgr()
    {
        return timer_mgr_ != nullptr ? timer_mgr_ : timer_mgr_ = GetWheelTimerMgr();
    }

    template <class Routine>
    static void RemoveRoutines(std::multimap<uint32, Routine*> &routines, uint32 type)
    {
        auto pair = routines.equal_range(type);
        for (auto itr = pair.first; itr != pair.second;) {
            delete itr++->second;
        }
    }

    template <class Routine>
    static void RemoveRoutines(std::multimap<uint32, Routine*> &routines)
    {
        auto pair = std::make_pair(routines.begin(), routines.end());
        for (auto itr = pair.first; itr != pair.second;) {
            delete itr++->second;
        }
    }

    template <class Routine>
    static bool HasRoutine(const std::multimap<uint32, Routine*> &routines, uint32 type)
    {
        return routines.find(type) != routines.end();
    }

    template <class Routine>
    static bool HasRoutine(const std::multimap<uint32, Routine*> &routines)
    {
        return !routines.empty();
    }

    template <class Routine>
    static Routine *FindRoutine(const std::multimap<uint32, Routine*> &routines, uint32 type)
    {
        typename std::multimap<uint32, Routine*>::const_iterator itr = routines.find(type);
        return itr != routines.end() ? itr->second : nullptr;
    }

private:
    WheelTimerMgr *timer_mgr_;
};

class WheelRoutineType : public noncopyable
{
public:
    WheelRoutineType(uint32 initial_type = 65536) : routine_type_(initial_type) {}

    uint32 NewUniqueRoutineType() { return routine_type_.fetch_add(1); }

private:
    std::atomic<uint32> routine_type_;
};
