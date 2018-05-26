#include "WheelTimer.h"
#include "WheelTimerMgr.h"

WheelTimer::WheelTimer(uint64 active_interval, uint32 loop_count)
: active_interval_(active_interval)
, active_tick_count_(0)
, loop_count_(loop_count)
, mgr_(nullptr)
{
}

WheelTimer::~WheelTimer()
{
    DetachMgr();
}

void WheelTimer::DetachMgr()
{
    if (mgr_ != nullptr) {
        mgr_->all_timer_[n1_][n2_].erase(itr_);
        mgr_ = nullptr;
    }
}

void WheelTimer::RePush(uint64 next_active_time)
{
    if (mgr_ != nullptr) {
        mgr_->RePush(this, next_active_time);
    }
}

void WheelTimer::SetLoop(uint32 loop_count)
{
    loop_count_ = loop_count;
}

void WheelTimer::SetNextActiveTime(uint64 next_active_time)
{
    if (next_active_time == 0) {
        uint64 active_tick_interval = active_interval_ / mgr_->tick_particle_;
        active_tick_count_ = active_tick_interval + mgr_->actual_tick_count_ - 1;
    } else {
        active_tick_count_ = next_active_time / mgr_->tick_particle_;
        if (active_tick_count_ < mgr_->tick_count_) {
            active_tick_count_ = mgr_->tick_count_;
        }
    }
}

void WheelTimer::FixFirstActiveTime()
{
    if (active_tick_count_ < mgr_->tick_count_) {
        active_tick_count_ = mgr_->tick_count_;
    }
}

uint64 WheelTimer::GetActualTickTime() const
{
    return mgr_->tick_particle_ * mgr_->actual_tick_count_;
}

uint64 WheelTimer::GetCurrentTickTime() const
{
    return mgr_->tick_particle_ * mgr_->tick_count_;
}

uint64 WheelTimer::GetNextActiveTime() const
{
    return mgr_->tick_particle_ * active_tick_count_;
}
