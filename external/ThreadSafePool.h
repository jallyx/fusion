#pragma once

#include <mutex>
#include "Concurrency.h"

template <typename T, size_t N>
class ThreadSafePool
{
public:
    ThreadSafePool()
        : index_(0)
    {}

    bool IsEmpty() const { return index_ == 0; }
    bool IsFull() const { return index_ == N; }
    size_t Size() const { return index_; }

    T *Get() {
        std::lock_guard<spinlock> lock(spin_);
        return index_ != 0 ? pool_[--index_] : nullptr;
    }

    bool Put(T *v) {
        std::lock_guard<spinlock> lock(spin_);
        if (index_ < N) {
            pool_[index_++] = v;
            return true;
        }
        return false;
    }

    void Clear() {
        std::lock_guard<spinlock> lock(spin_);
        index_ = 0;
    }

private:
    T *pool_[N];
    size_t index_;
    spinlock spin_;
};
