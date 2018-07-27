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

template <typename T> inline T DivWithCeil(T dividend, T divisor) {
    return (dividend / divisor) + (dividend % divisor ? 1 : 0);
}
template <typename T> inline T SubLeastZero(T minuend, T subtrahend) {
    return minuend > subtrahend ? minuend - subtrahend : 0;
}
template <typename T> inline void SubLeastZeroX(T &baseValue, T paramValue) {
    baseValue = SubLeastZero(baseValue, paramValue);
}
template <typename T, typename X> inline T AddLeastZero(T augend, X addend) {
    return addend >= 0 || augend > T(-addend) ? T(augend + addend) : 0;
}
template <typename T, typename X> inline void AddLeastZeroX(T &baseValue, X paramValue) {
    baseValue = AddLeastZero(baseValue, paramValue);
}

template <typename T, typename... Args> T *REINIT_OBJECT(T *obj, Args... args) {
    obj->~T(); return new(obj) T(std::forward<Args>(args)...);
}
