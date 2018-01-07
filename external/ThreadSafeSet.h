#pragma once

#include <mutex>
#include <unordered_set>

template <typename T>
class ThreadSafeSet
{
public:
    bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return set_.empty();
    }

    size_t GetSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return set_.size();
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        set_.clear();
    }

    bool Insert(T v) {
        std::lock_guard<std::mutex> lock(mutex_);
        return set_.insert(v).second;
    }

    bool Remove(T v) {
        std::lock_guard<std::mutex> lock(mutex_);
        return set_.erase(v) != 0;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_set<T> set_;
};
