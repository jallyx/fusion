#pragma once

#include "WheelRoutineOwner.h"
#include <functional>
#include "enable_linked_from_this.h"

class WheelTimerOwner : virtual private WheelRoutineOwner,
    public enable_linked_from_this<WheelTimerOwner>
{
public:
    WheelTimerOwner();
    virtual ~WheelTimerOwner();

    void CreateTimer(const std::function<void()> &cb, uint32 type,
        uint64 interval, uint32 repeats = 0, uint64 firstime = 0);
    void CreateTimerX(const std::function<void()> &cb,
        uint64 interval, uint32 repeats = 0, uint64 firstime = 0);

    void RemoveTimers(uint32 type);
    void RemoveTimers();
    bool HasTimer(uint32 type) const;
    bool HasTimer() const;
    template <typename T> T *FindTimer(uint32 type) const;

private:
    class Timer;
    std::list<Timer*> timer_list_;
};
