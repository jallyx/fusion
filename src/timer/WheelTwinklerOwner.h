#pragma once

#include "WheelRoutineOwner.h"
#include "RoutineTrigger.h"
#include <functional>
#include "enable_linked_from_this.h"

class WheelTwinklerOwner : virtual protected WheelRoutineOwner,
    public enable_linked_from_this<WheelTwinklerOwner>
{
public:
    WheelTwinklerOwner();
    ~WheelTwinklerOwner();

    void CreateTwinkler(TriggerCycle trigger_cycle, TriggerPoint trigger_point,
        time_t trigger_duration, const std::function<void()> &start_cb,
        const std::function<void()> &stop_cb, uint32 routine_type,
        bool is_isolate = false, bool is_restore = false,
        uint32 repeats = 0);
    void CreateTwinklerX(TriggerCycle trigger_cycle, TriggerPoint trigger_point,
        time_t trigger_duration, const std::function<void()> &start_cb,
        const std::function<void()> &stop_cb, bool is_isolate = false,
        bool is_restore = false, uint32 repeats = 0);

    void CreateTwinkler(uint64 trigger_interval, uint64 trigger_point,
        uint64 trigger_duration, const std::function<void()> &start_cb,
        const std::function<void()> &stop_cb, uint32 routine_type,
        bool is_strict = true, bool is_isolate = false,
        bool is_restore = false, uint32 repeats = 0);
    void CreateTwinklerX(uint64 trigger_interval, uint64 trigger_point,
        uint64 trigger_duration, const std::function<void()> &start_cb,
        const std::function<void()> &stop_cb, bool is_strict = true,
        bool is_isolate = false, bool is_restore = false,
        uint32 repeats = 0);

    void RemoveTwinklers(uint32 type);
    void RemoveTwinklers();
    bool HasTwinkler(uint32 type) const;
    bool HasTwinkler() const;
    template <typename T> T *FindTwinkler(uint32 type) const;

private:
    class Twinkler;
    class TwinklerByMonthly;
    std::multimap<uint32, Twinkler*> twinklers_;
};
