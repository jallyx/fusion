#pragma once

#include <stddef.h>
#include <list>
#include "Base.h"

class WheelTimerMgr;

class WheelTimer
{
    friend class WheelTimerMgr;
public:
    WheelTimer(uint64 active_interval, uint32 loop_count = 0);

    void RePush(uint64 next_active_time = 0);

    void DetachMgr();

    uint64 GetCurrentTickTime() const;
    uint64 GetNextActiveTime() const;

    uint32 GetRemainder() const { return loop_count_; }

    uint64 active_interval() const { return active_interval_; }

protected:
    virtual ~WheelTimer();
    virtual bool OnPrepare() { return true; }
    virtual void OnActivate() = 0;
    virtual void RemoveRelevance() { mgr_ = nullptr; }
    void SetNextActiveTime(uint64 next_active_time = 0);
    void FixFirstActiveTime();

    void Invalidate() {
        loop_count_ = 1;
    }
    void IgnoreOnceLoop() {
        if (loop_count_ != 0)
            ++loop_count_;
    }
    void ReActivate() {
        SetNextActiveTime(1);
    }

private:
    const uint64 active_interval_;
    uint64 active_tick_count_;
    uint32 loop_count_;

    size_t n1_, n2_;
    std::list<WheelTimer*>::iterator itr_;
    WheelTimerMgr *mgr_;
};
