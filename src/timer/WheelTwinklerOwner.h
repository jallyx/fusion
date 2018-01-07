#pragma once

#include "WheelRoutineOwner.h"
#include "RoutineTrigger.h"
#include <functional>
#include "enable_linked_from_this.h"

class WheelTwinklerOwner : virtual private WheelRoutineOwner,
    public enable_linked_from_this<WheelTwinklerOwner>
{
public:
    WheelTwinklerOwner();
    virtual ~WheelTwinklerOwner();

    void CreateTwinkler(TriggerCycle trigger_cycle, TriggerPoint trigger_point,
        time_t trigger_duration, const std::function<void()> &start_cb,
        const std::function<void()> &stop_cb, uint32 routine_type,
        bool is_isolate = false, bool is_restore = false,
        uint32 repeats = 0);
    void CreateTwinklerX(TriggerCycle trigger_cycle, TriggerPoint trigger_point,
        time_t trigger_duration, const std::function<void()> &start_cb,
        const std::function<void()> &stop_cb, bool is_isolate = false,
        bool is_restore = false, uint32 repeats = 0);

    void CreateTwinkler(time_t trigger_interval, time_t trigger_point,
        time_t trigger_duration, const std::function<void()> &start_cb,
        const std::function<void()> &stop_cb, uint32 routine_type,
        bool is_strict = true, bool is_isolate = false,
        bool is_restore = false, uint32 repeats = 0);
    void CreateTwinklerX(time_t trigger_interval, time_t trigger_point,
        time_t trigger_duration, const std::function<void()> &start_cb,
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
    std::list<Twinkler*> twinkler_list_;
};
