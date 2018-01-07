#pragma once

inline void noop_deleter(void *ptr) {}

template <typename T> void BIT_CLR(T &value, int pos) {
    value &= ~(static_cast<T>(1) << pos);
}
template <typename T> void BIT_SET(T &value, int pos) {
    value |= (static_cast<T>(1) << pos);
}
template <typename T> bool BIT_ISSET(T &value, int pos) {
    return (value & (static_cast<T>(1) << pos)) != 0;
}

template <class InputIt, class Distance>
InputIt Advance(InputIt it, Distance n) {
    for (; n > 0; --n) ++it;
    for (; n < 0; ++n) --it;
    return it;
}
