#pragma once

#include "NetStream.h"
#include <atomic>

class ConstNetBuffer : public INetStream
{
public:
    ConstNetBuffer(const void *data, size_t size) {
        InitInternalBuffer((char*)data, size);
        Erlarge(size);
    }
};

template <size_t N>
class TNetBuffer : public INetStream
{
public:
    template <typename... Args>
    TNetBuffer(Args&&... args) : TNetBuffer() {
        swallow{ 0, (operator<<(std::forward<Args>(args)), 0)... };
    }
    TNetBuffer() {
        InitInternalBuffer(buffer_internal_, sizeof(buffer_internal_));
#if defined(ENABLE_PROFILER)
        s_total_.fetch_add(1);
#endif
    }
    virtual ~TNetBuffer() {
#if defined(ENABLE_PROFILER)
        s_total_.fetch_sub(1);
#endif
    }

private:
    char buffer_internal_[N];
    static std::atomic_long s_total_;
};

template <size_t N>
std::atomic_long TNetBuffer<N>::s_total_(0);

typedef TNetBuffer<256> NetBuffer;
