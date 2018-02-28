#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

// be useful for:
// multi producer, single consumer.

template <typename T>
class ThreadSafeBlockQueue
{
public:
    ThreadSafeBlockQueue()
        : enptr_(queue_)
    {}

    void Enqueue(const T &v) {
        std::lock_guard<std::mutex> lock(mutex_);
        (*enptr_).push(v);
        cv_.notify_one();
    }

    bool Dequeue(T &v, long timeout_ms) {
        if (DequeueOutPool(v)) {
            return true;
        }
        do {
            std::lock_guard<std::mutex> lock(mutex_);
            enptr_ = enptr_ != queue_ ? &queue_[0] : &queue_[1];
        } while (0);
        if (DequeueOutPool(v)) {
            return true;
        }
        do {
            std::unique_lock<std::mutex> lock(mutex_);
            if (DequeueInPool(v)) {
                return true;
            }
            const std::chrono::milliseconds duration(timeout_ms);
            if (cv_.wait_for(lock, duration) == std::cv_status::no_timeout) {
                if (DequeueInPool(v)) {
                    return true;
                }
            }
        } while (0);
        return false;
    }

private:
    bool DequeuePool(T &v, std::queue<T> &pool) {
        if (!pool.empty()) {
            v = pool.front();
            pool.pop();
            return true;
        }
        return false;
    }

    bool DequeueInPool(T &v) {
        return DequeuePool(v, *enptr_);
    }
    bool DequeueOutPool(T &v) {
        return DequeuePool(v, enptr_ != queue_ ? queue_[0] : queue_[1]);
    }

    std::condition_variable cv_;
    std::mutex mutex_;
    std::queue<T> queue_[2];
    std::queue<T> *enptr_;
};
