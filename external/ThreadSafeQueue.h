#pragma once

#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue
{
public:
    bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    size_t GetSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void Enqueue(T v) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(v);
    }

    bool Dequeue(T &v) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!queue_.empty()) {
            v = queue_.front();
            queue_.pop();
            return true;
        }
        return false;
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
};
