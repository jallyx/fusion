#include "WheelTimerMgr.h"
#include "WheelTimer.h"
#include "Exception.h"
#include "Logger.h"
#include "Macro.h"

static const size_t capacity[] = {256,64,64,64,64};

WheelTimerMgr::WheelTimerMgr(uint64 particle, uint64 curtime)
: tick_particle_(particle)
, tick_count_(curtime/particle)
, actual_tick_count_(curtime/particle)
, anchor_container_{nullptr}
{
    pointer_slot_.resize(ARRAY_SIZE(capacity));
    all_timer_.resize(ARRAY_SIZE(capacity));
    for (size_t i = 0; i < ARRAY_SIZE(capacity); ++i) {
        all_timer_[i].resize(capacity[i]);
    }
}

WheelTimerMgr::~WheelTimerMgr()
{
    Clear();
}

void WheelTimerMgr::Clear()
{
    while (!push_pool_.empty()) {
        WheelTimer *timer = push_pool_.front();
        push_pool_.pop_front();
        pop_pool_.remove(timer);
        timer->RemoveRelevance();
        timer->mgr_ = nullptr;
        delete timer;
    }
    while (!pop_pool_.empty()) {
        WheelTimer *timer = pop_pool_.front();
        pop_pool_.pop_front();
        delete timer;
    }
    for (size_t i = 0; i < ARRAY_SIZE(capacity); ++i) {
        for (size_t j = 0; j < capacity[i]; ++j) {
            std::list<WheelTimer*> &tlist = all_timer_[i][j];
            while (!tlist.empty()) {
                delete tlist.front();
            }
        }
    }
}

void WheelTimerMgr::Update(uint64 curtime)
{
    actual_tick_count_ = curtime / tick_particle_;
    while (tick_count_ < actual_tick_count_) {
        DynamicMerge();
        DynamicRemove();
        CascadeAndTick();
        Relocate(Activate());
    }

    if (thread_id_ == std::thread::id()) {
        thread_id_ = std::this_thread::get_id();
    }
}

void WheelTimerMgr::RePush(WheelTimer *timer, uint64 next_active_time)
{
    if (thread_id_ == std::this_thread::get_id()) {
        std::list<WheelTimer*> timer_list;
        timer_list.splice(timer_list.end(),
            all_timer_[timer->n1_][timer->n2_], timer->itr_);
        timer->SetNextActiveTime(next_active_time);
        Relocate(std::move(timer_list));
    }
}

void WheelTimerMgr::Push(WheelTimer *timer, uint64 first_active_time)
{
    timer->mgr_ = this;
    timer->SetNextActiveTime(first_active_time);

    do {
        std::lock_guard<std::mutex> lock(push_mutex_);
        push_pool_.push_back(timer);
    } while (0);

    if (thread_id_ == std::this_thread::get_id()) {
        DynamicMerge();
    }
}

void WheelTimerMgr::Pop(WheelTimer *timer)
{
    do {
        std::lock_guard<std::mutex> lock(pop_mutex_);
        pop_pool_.push_back(timer);
    } while (0);

    if (thread_id_ == std::this_thread::get_id()) {
        DynamicRemove();
    }
}

void WheelTimerMgr::DynamicMerge()
{
    std::list<WheelTimer*> timer_list;
    do {
        std::lock_guard<std::mutex> lock(push_mutex_);
        timer_list.swap(push_pool_);
    } while (0);

    for (auto itr = timer_list.begin(); itr != timer_list.end();) {
        WheelTimer *timer = *itr;
        if (timer->OnPrepare()) {
            timer->FixFirstActiveTime();
            ++itr;
        } else {
            itr = timer_list.erase(itr);
            timer->mgr_ = nullptr;
            delete timer;
        }
    }

    Relocate(std::move(timer_list));
}

void WheelTimerMgr::DynamicRemove()
{
    std::list<WheelTimer*> timer_list;
    do {
        std::lock_guard<std::mutex> lock(pop_mutex_);
        timer_list.swap(pop_pool_);
    } while (0);

    for (auto timer : timer_list) {
        delete timer;
    }
}

void WheelTimerMgr::CascadeAndTick()
{
    if (1 + pointer_slot_[0] >= capacity[0]) {
        std::list<WheelTimer*> pending_timer_list;
        for (size_t index = 1; index < ARRAY_SIZE(capacity); ++index) {
            size_t linear = pointer_slot_[index] + 1;
            size_t slot = linear < capacity[index] ? linear : 0;
            pending_timer_list.splice(
                pending_timer_list.end(), all_timer_[index][slot]);
            if (slot != 0) {
                break;
            }
        }
        Relocate(std::move(pending_timer_list));
    }

    for (size_t index = 0; index < ARRAY_SIZE(capacity); ++index) {
        if (++pointer_slot_[index] >= capacity[index]) {
            pointer_slot_[index] = 0;
        } else {
            break;
        }
    }

    ++tick_count_;
}

std::list<WheelTimer*> WheelTimerMgr::Activate()
{
    std::list<WheelTimer*> &active_timer_list = all_timer_[0][pointer_slot_[0]];
    if (!active_timer_list.empty()) {
        std::list<WheelTimer*>::iterator itr = active_timer_list.begin();
        std::list<WheelTimer*>::iterator anchor = anchor_container_.begin();
        active_timer_list.splice(itr, anchor_container_, anchor);
        do {
            WheelTimer *timer = *itr;
            timer->SetNextActiveTime();
            TRY_BEGIN {
                timer->OnActivate();
            } TRY_END
            CATCH_BEGIN(const IException &e) {
                e.Print();
            } CATCH_END
            CATCH_BEGIN(...) {
            } CATCH_END
            itr = std::next(anchor);
            if (itr != active_timer_list.end() && *itr == timer) {
                active_timer_list.splice(++itr, active_timer_list, anchor);
                switch (timer->loop_count_) {
                case 0:
                    break;
                case 1:
                    delete timer;
                    break;
                default:
                    --timer->loop_count_;
                    break;
                }
            }
        } while (itr != active_timer_list.end());
        anchor_container_.splice(anchor_container_.end(), active_timer_list, anchor);
    }
    return std::move(active_timer_list);
}

void WheelTimerMgr::Relocate(std::list<WheelTimer*> &&pending_timer_list)
{
    while (!pending_timer_list.empty()) {
        WheelTimer *timer = pending_timer_list.front();
        timer->mgr_ = nullptr;
        uint64 evaluate_value = timer->active_tick_count_ - tick_count_;
        for (size_t index = 0; index < ARRAY_SIZE(capacity); ++index) {
            if (evaluate_value >= capacity[index]) {
                uint64 linear = evaluate_value - capacity[index] + pointer_slot_[index] + 1;
                evaluate_value = linear / capacity[index];
            } else {
                uint64 linear = pointer_slot_[index] + evaluate_value + 1;
                size_t slot = linear % capacity[index];
                std::list<WheelTimer*> &tlist = all_timer_[index][slot];
                tlist.splice(tlist.end(), pending_timer_list, pending_timer_list.begin());
                timer->n1_ = index;
                timer->n2_ = slot;
                timer->itr_ = --tlist.end();
                timer->mgr_ = this;
                break;
            }
        }
        if (timer->mgr_ == nullptr) {
            ELOG("Relocate wheel timer(%u,%u) error.", timer->active_tick_count_, tick_count_);
            pending_timer_list.pop_front();
            delete timer;
        }
    }
}
