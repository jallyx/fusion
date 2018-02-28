#pragma once

#include <string.h>
#include <algorithm>
#include "Base.h"
#include "Macro.h"
#include "Debugger.h"
#include "InlineFuncs.h"

// be useful for:
// single producer, single consumer.

#define UDP_RECV_BUFFER_MAX (256*1024)

#define MAX_RECV_QUEUE_POOL_MEMORY (8*1024*1024)
#define MAX_RECV_QUEUE_POOL_COUNT (4096)
#define S_MAX_RECV_QUEUE_POOL_COUNT(N) \
    MAX(MIN(MAX_RECV_QUEUE_POOL_MEMORY/N, MAX_RECV_QUEUE_POOL_COUNT), 1)

template <size_t N>
class TRecvQueue
{
    struct DataBuffer {
        DataBuffer *next = nullptr;
        size_t rpos = 0, wpos = 0;
        char buffer[N];
    };

    struct PktInfo {
        uint64 seq;
        size_t size;
    };

public:
    TRecvQueue() : head_(AllocBuffer()), cap_(N), seq_(0) {
        tail_ = frag_ = head_;
    }
    ~TRecvQueue() {
        do {
            auto next = head_->next;
            FreeBuffer(head_);
            head_ = next;
        } while (head_ != nullptr);
    }

    bool HasRecvDataAvail() const {
        return head_->wpos > head_->rpos;
    }

    const char *GetRecvDataBuffer(size_t &size) {
        if (head_->wpos > head_->rpos) {
            size = head_->wpos - head_->rpos;
            return head_->buffer + head_->rpos;
        } else {
            return nullptr;
        }
    }

    void RemoveRecvData(size_t size) {
        DBGASSERT(head_->rpos + size <= head_->wpos);
        head_->rpos += size;
        if (head_->rpos >= N) {
            auto next = head_->next;
            FreeBuffer(head_);
            head_ = next;
        }
    }

    void OnPktRecv(const char *data, size_t size, uint64 seq) {
        if (TryRecvPkt(size, seq)) {
            auto skip = size_t(seq - seq_);
            Reserve(skip + size);
            Insert(data, size, skip);
            if (skip == 0) {
                Merge();
            }
        }
    }

    bool overflow(size_t size, uint64 seq) const {
        return seq + size > seq_ + UDP_RECV_BUFFER_MAX;
    }

    uint64 seq() const { return seq_; }

private:
    bool TryRecvPkt(size_t size, uint64 seq) {
        if (seq < seq_ || overflow(size, seq))
            return false;
        auto itr = std::find_if(pkts_.begin(), pkts_.end(),
            [=](const PktInfo &info) { return seq <= info.seq; });
        if (itr != pkts_.end() && itr->seq == seq)
            return false;
        pkts_.insert(itr, {seq, size});
        return true;
    }

    void Reserve(size_t cap) {
        for (; cap_ <= cap; cap_ += N) {
            tail_->next = AllocBuffer();
            tail_ = tail_->next;
        }
    }

    void Insert(const char *data, size_t size, size_t skip) {
        DataBuffer *ptr = frag_;
        size_t pos = frag_->wpos;
        while (skip > 0) {
            size_t avail = std::min(N - pos, skip);
            pos += avail, skip -= avail;
            if (pos >= N) {
                pos = 0, ptr = ptr->next;
            }
        }
        while (size > 0) {
            size_t avail = std::min(N - pos, size);
            memcpy(ptr->buffer + pos, data, avail);
            pos += avail, data += avail, size -= avail;
            if (pos >= N) {
                pos = 0, ptr = ptr->next;
            }
        }
    }

    void Merge() {
        for (; !pkts_.empty(); pkts_.pop_front()) {
            auto &pkt = pkts_.front();
            if (pkt.seq == seq_) {
                cap_ -= pkt.size, seq_ += pkt.size;
                DataBuffer *next = frag_->next;
                size_t pos = frag_->wpos;
                do {
                    size_t avail = std::min(N - pos, pkt.size);
                    frag_->wpos += avail, pkt.size -= avail;
                    if ((pos += avail) >= N) {
                        frag_ = next, next = frag_->next, pos = 0;
                    }
                } while (pkt.size > 0);
            } else if (pkt.seq < seq_) {
                continue;
            } else {
                break;
            }
        }
    }

    DataBuffer *head_, *frag_, *tail_;
    size_t cap_;
    uint64 seq_;
    std::deque<PktInfo> pkts_;

public:
    static void InitBufferPool() {
    }
    static void ClearBufferPool() {
        DataBuffer *buffer = nullptr;
        while ((buffer = buffer_pool_.Get()) != nullptr) {
            delete buffer;
        }
    }

private:
    static DataBuffer *AllocBuffer() {
        DataBuffer *buffer = nullptr;
        if ((buffer = buffer_pool_.Get()) != nullptr) {
            return REINIT_OBJECT(buffer);
        } else {
            return new DataBuffer;
        }
    }
    static void FreeBuffer(DataBuffer *buffer) {
        if (!buffer_pool_.Put(buffer)) {
            delete buffer;
        }
    }

    static ThreadSafePool<DataBuffer, S_MAX_RECV_QUEUE_POOL_COUNT(N)>
        buffer_pool_;
};

template <size_t N>
ThreadSafePool<
    typename TRecvQueue<N>::DataBuffer,
    S_MAX_RECV_QUEUE_POOL_COUNT(N)
> TRecvQueue<N>::buffer_pool_;

typedef TRecvQueue<65536> RecvQueue;
