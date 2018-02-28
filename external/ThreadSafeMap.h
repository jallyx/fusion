#pragma once

#include <mutex>
#include <unordered_map>

template <typename K, typename V>
class ThreadSafeMap
{
public:
    bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.empty();
    }

    size_t GetSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
    }

    bool Insert(const K &k, const V &v) {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.emplace(k, v).second;
    }

    bool Remove(const K &k, V &v) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto itr = map_.find(k);
        if (itr != map_.end()) {
            v = std::move(itr->second);
            map_.erase(itr);
            return true;
        } else {
            return false;
        }
    }

    bool Get(const K &k, V &v) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto itr = map_.find(k);
        if (itr != map_.end()) {
            v = itr->second;
            return true;
        } else {
            return false;
        }
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<K, V> map_;
};
