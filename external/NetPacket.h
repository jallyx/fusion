#pragma once

#include "NetStream.h"
#include <atomic>
#include "Macro.h"
#include "ThreadSafePool.h"

#define MAX_NET_PACKET_SIZE (65535)

#define MAX_NET_PACKET_POOL_MEMORY (8*1024*1024)
#define MAX_NET_PACKET_POOL_COUNT (4096)
#define S_MAX_NET_PACKET_POOL_COUNT(N) \
    MAX(MIN(MAX_NET_PACKET_POOL_MEMORY/N, MAX_NET_PACKET_POOL_COUNT), 1)

class INetPacket : public INetStream
{
public:
    struct Header {
        uint32 cmd;
        size_t len;
        Header() = default;
        Header(uint32 c, size_t l) : cmd(c), len(l) {}
        static const size_t SIZE = 4;
    };
    static const size_t MAX_BUFFER_SIZE = MAX_NET_PACKET_SIZE - Header::SIZE;

    INetPacket(uint32 opcode)
        : opcode_(opcode)
    {}

    void Reset(uint32 opcode) {
        SetOpcode(opcode);
        Clear();
    }

    INetPacket *Clone() const;

    INetPacket *ReadPacket();
    INetPacket &ReadPacket(INetPacket &pck);

    INetPacket &UnpackPacket();

    void WritePacket(const INetPacket &other);

    void WriteHeader(const Header &header);
    void ReadHeader(Header &header);

    void SetOpcode(uint32 opcode) { opcode_ = opcode; }
    uint32 GetOpcode() const { return opcode_; }

    static void InitNetPacketPool();
    static void ClearNetPacketPool();
    static INetPacket *New(uint32 opcode, size_t size);

private:
    uint32 opcode_;
};

class ConstNetPacket : public INetPacket
{
public:
    ConstNetPacket(const void *data, size_t size, uint32 opcode = 0)
        : INetPacket(opcode)
    {
        InitInternalBuffer((char*)data, size);
        Erlarge(size);
    }
};

template <size_t N>
class TNetPacket : public INetPacket
{
public:
    TNetPacket(uint32 opcode = 0) : INetPacket(opcode) {
        InitInternalBuffer(buffer_internal_, sizeof(buffer_internal_));
#if defined(ENABLE_PROFILER)
        s_total_.fetch_add(1);
#endif
    }
    virtual ~TNetPacket() {
#if defined(ENABLE_PROFILER)
        s_total_.fetch_sub(1);
#endif
    }

    static void *operator new(size_t size) {
        void *pck = s_pool_.Get();
        if (pck == nullptr) {
            pck = ::operator new(size);
        }
        return pck;
    }
    static void operator delete(void *pck) {
        if (!s_pool_.Put(pck)) {
            ::operator delete(pck);
        }
    }

    static void InitPool() {
    }
    static void ClearPool() {
        void *pck = nullptr;
        while ((pck = s_pool_.Get()) != nullptr) {
            ::operator delete(pck);
        }
    }

private:
    char buffer_internal_[N];
    static std::atomic_long s_total_;
    static ThreadSafePool<void, S_MAX_NET_PACKET_POOL_COUNT(N)> s_pool_;
};

template <size_t N>
std::atomic_long TNetPacket<N>::s_total_(0);
template <size_t N>
ThreadSafePool<void, S_MAX_NET_PACKET_POOL_COUNT(N)> TNetPacket<N>::s_pool_;

typedef TNetPacket<256> NetPacket;
