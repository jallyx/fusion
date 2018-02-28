#pragma once

#include <utility>

inline void noop_deleter(void *ptr) {}

template <typename T> void BIT_CLR(T &value, int pos) {
    value &= ~(static_cast<T>(1) << pos);
}
template <typename T> void BIT_SET(T &value, int pos) {
    value |= (static_cast<T>(1) << pos);
}
template <typename T> bool BIT_ISSET(const T &value, int pos) {
    return (value & (static_cast<T>(1) << pos)) != 0;
}

template <typename T, typename... Args> T *REINIT_OBJECT(T *obj, Args... args) {
    obj->~T(); return new(obj) T(std::forward<Args>(args)...);
}
