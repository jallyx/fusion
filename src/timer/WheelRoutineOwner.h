#pragma once

#include <list>
#include "Base.h"

class WheelTimerMgr;

class WheelRoutineOwner
{
public:
    WheelRoutineOwner() : timer_mgr_(nullptr) {}
    virtual ~WheelRoutineOwner() {}

protected:
    virtual WheelTimerMgr *GetWheelTimerMgr() = 0;

    inline WheelTimerMgr *GetCacheWheelTimerMgr()
    {
        if (timer_mgr_ == nullptr)
            timer_mgr_ = GetWheelTimerMgr();
        return timer_mgr_;
    }

    template <class Routine>
    static void RemoveRoutines(std::list<Routine*> &routine_list, uint32 type)
    {
        typename std::list<Routine*>::iterator iterator = routine_list.begin();
        while (iterator != routine_list.end()) {
            Routine *routine = *iterator;
            ++iterator;
            if (routine->GetRoutineType() == type)
                delete routine;
        }
    }

    template <class Routine>
    static void RemoveRoutines(std::list<Routine*> &routine_list)
    {
        while (!routine_list.empty())
            delete routine_list.front();
    }

    template <class Routine>
    static bool HasRoutine(const std::list<Routine*> &routine_list, uint32 type)
    {
        typename std::list<Routine*>::const_iterator iterator = routine_list.begin();
        for (; iterator != routine_list.end(); ++iterator) {
            if ((*iterator)->GetRoutineType() == type)
                return true;
        }
        return false;
    }

    template <class Routine>
    static bool HasRoutine(const std::list<Routine*> &routine_list)
    {
        return !routine_list.empty();
    }

    template <class Routine>
    static Routine *FindRoutine(const std::list<Routine*> &routine_list, uint32 type)
    {
        typename std::list<Routine*>::const_iterator iterator = routine_list.begin();
        for (; iterator != routine_list.end(); ++iterator) {
            if ((*iterator)->GetRoutineType() == type)
                return *iterator;
        }
        return nullptr;
    }

private:
    WheelTimerMgr *timer_mgr_;
};
