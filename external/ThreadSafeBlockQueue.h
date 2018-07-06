#pragma once

#include <condition_variable>
#include "MultiBufferQueue.h"

// be useful for:
// multi producer, single consumer.

template <typename T, size_t N>
class ThreadSafeBlockQueue
{
public:
    void Enqueue(const T &v) {
        data_queue_.Enqueue(v);
        cv_.notify_one();
    }

    bool Dequeue(T &v, long timeout_ms) {
        if (data_queue_.Swap() && data_queue_.Dequeue(v)) {
            return true;
        }
        const std::chrono::milliseconds duration(timeout_ms);
        if (cv_.wait_for(fakelock_, duration) == std::cv_status::no_timeout) {
            if (data_queue_.Swap() && data_queue_.Dequeue(v)) {
                return true;
            }
        }
        return false;
    }

private:
    fakelock fakelock_;
    std::condition_variable_any cv_;
    MultiBufferQueue<T, N> data_queue_;
};
