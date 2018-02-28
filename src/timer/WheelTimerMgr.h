#pragma once

#include <list>
#include <mutex>
#include <thread>
#include <vector>
#include "Base.h"

class WheelTimer;

class WheelTimerMgr
{
    friend class WheelTimer;
public:
    WheelTimerMgr(uint64 particle, uint64 curtime);
    ~WheelTimerMgr();

    void Update(uint64 curtime);

    void RePush(WheelTimer *timer, uint64 next_active_time = 0);
    void Push(WheelTimer *timer, uint64 first_active_time = 0);
    void Pop(WheelTimer *timer);

private:
    void DynamicMerge();
    void DynamicRemove();

    void CascadeAndTick();

    std::list<WheelTimer*> Activate();
    void Relocate(std::list<WheelTimer*> &&pending_timer_list);

    const uint64 tick_particle_;
    std::thread::id thread_id_;

    uint64 tick_count_;
    std::vector<size_t> pointer_slot_;
    std::vector<std::vector<std::list<WheelTimer*>>> all_timer_;

    std::list<WheelTimer*> push_pool_, pop_pool_;
    std::mutex push_mutex_, pop_mutex_;

    std::list<WheelTimer*> anchor_container_;
};
