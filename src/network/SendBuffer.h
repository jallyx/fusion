#pragma once

#include "Debugger.h"
#include "InlineFuncs.h"
#include "NetPacket.h"

// be useful for:
// multi producer, single consumer.

#define MAX_SEND_BUFFER_POOL_MEMORY (8*1024*1024)
#define MAX_SEND_BUFFER_POOL_COUNT (4096)
#define S_MAX_SEND_BUFFER_POOL_COUNT(N) \
    MAX(MIN(MAX_SEND_BUFFER_POOL_MEMORY/N, MAX_SEND_BUFFER_POOL_COUNT), 1)

template <size_t N>
class TSendBuffer
{
    struct DataBuffer {
        DataBuffer *next = nullptr;
        size_t rpos = 0, wpos = 0;
        char buffer[N];
    };
    struct DataWriter {
        DataWriter *prev, *next = nullptr;
        DataBuffer *ptr[2], *eptr = nullptr;
        size_t n, pos[2], epos = 0;
    };
    struct DataPending {
        DataWriter *writer = nullptr;
        DataBuffer *buffer;
        size_t epos = 0;
    };

    struct DataWriterHelper {
        DataWriterHelper(TSendBuffer &This, size_t n)
            : This(This) { w.n = n; This.Prepare(w); }
        ~DataWriterHelper() {
            DBGASSERT(w.ptr[0] == w.ptr[1]);
            DBGASSERT(w.pos[0] == w.pos[1]);
            DBGASSERT(w.n == 0);
            This.Flush(w);
        }
        TSendBuffer &This;
        DataWriter w;
    };

public:
    TSendBuffer() : head_(AllocBuffer()), size_(0) {
        pending_.buffer = tail_ = head_;
    }
    ~TSendBuffer() {
        do {
            auto next = head_->next;
            FreeBuffer(head_);
            head_ = next;
        } while (head_ != nullptr);
    }

    const char *GetSendDataBuffer(size_t &size) {
        if (head_->wpos > head_->rpos) {
            size = head_->wpos - head_->rpos;
            return head_->buffer + head_->rpos;
        } else {
            return nullptr;
        }
    }
    void RemoveSendData(size_t size) {
        DBGASSERT(head_->rpos + size <= head_->wpos);
        head_->rpos += size, size_.fetch_sub(size);
        if (head_->rpos >= N) {
            auto next = head_->next;
            FreeBuffer(head_);
            head_ = next;
        }
    }

    void WritePacket(const INetPacket &pck) {
        DataWriterHelper _(*this,
            INetPacket::Header::SIZE + pck.GetReadableSize());
        Header(pck.GetOpcode(), _.w.n, _.w);
        Append(pck.GetReadableBuffer(), pck.GetReadableSize(), _.w);
    }
    void WritePacket(const char *data, size_t size) {
        DataWriterHelper _(*this, size);
        Append(data, size, _.w);
    }
    void WritePacket(const INetPacket &pck, const INetPacket &data) {
        DataWriterHelper _(*this,
            INetPacket::Header::SIZE + pck.GetReadableSize() +
            INetPacket::Header::SIZE + data.GetReadableSize());
        Header(pck.GetOpcode(), _.w.n, _.w);
        Append(pck.GetReadableBuffer(), pck.GetReadableSize(), _.w);
        Header(data.GetOpcode(), _.w.n, _.w);
        Append(data.GetReadableBuffer(), data.GetReadableSize(), _.w);
    }
    void WritePacket(const INetPacket &pck, const char *data, size_t size) {
        DataWriterHelper _(*this, size +
            INetPacket::Header::SIZE + pck.GetReadableSize());
        Header(pck.GetOpcode(), _.w.n, _.w);
        Append(pck.GetReadableBuffer(), pck.GetReadableSize(), _.w);
        Append(data, size, _.w);
    }

    bool HasDataAwaiting() const { return size_.load() != 0; }
    size_t GetDataSize() const { return size_.load(); }

private:
    void Header(uint32 cmd, size_t len, DataWriter &w) {
        TNetPacket<INetPacket::Header::SIZE> wrapper;
        wrapper.WriteHeader(INetPacket::Header(cmd, len));
        Append(wrapper.GetBuffer(), wrapper.GetTotalSize(), w);
    }

    void Append(const char *data, size_t size, DataWriter &w) {
        while (size > 0) {
            auto avail = std::min(N - w.pos[0], size);
            memcpy(w.ptr[0]->buffer + w.pos[0], data, avail);
            w.n -= avail, size_.fetch_add(avail);
            w.pos[0] += avail, data += avail, size -= avail;
            if (w.pos[0] >= N) {
                w.ptr[0]->next = w.n < N ? w.ptr[1] : AllocBuffer();
                w.ptr[0] = w.ptr[0]->next;
                w.pos[0] = 0;
            }
        }
    }

    void Prepare(DataWriter &w) {
        DataBuffer *buffer = nullptr;
        if (pending_.epos + w.n >= N) {
mark:       buffer = AllocBuffer();
        }
        bool isOk = false;
        do {
            std::lock_guard<spinlock> lock(spin_);
            if (pending_.epos + w.n < N) {
                w.ptr[1] = pending_.buffer;
                w.pos[1] = pending_.epos + w.n;
                isOk = true;
            } else if (buffer != nullptr) {
                w.ptr[1] = buffer, buffer = nullptr;
                w.pos[1] = (pending_.epos + w.n) % N;
                isOk = true;
            }
            if (isOk) {
                if (pending_.writer != nullptr) {
                    pending_.writer->next = &w;
                }
                w.prev = pending_.writer;
                w.ptr[0] = pending_.buffer;
                w.pos[0] = pending_.epos;
                pending_.writer = &w;
                pending_.buffer = w.ptr[1];
                pending_.epos = w.pos[1];
            }
        } while (0);
        if (isOk) {
            if (buffer != nullptr) {
                FreeBuffer(buffer);
            }
        } else {
            goto mark;
        }
    }
    void Flush(DataWriter &w) {
        do {
            std::lock_guard<spinlock> lock(spin_);
            if (w.eptr != nullptr) {
                w.ptr[1] = w.eptr;
                w.pos[1] = w.epos;
            }
            if (w.prev == nullptr) {
                w.ptr[0] = tail_, tail_ = w.ptr[1];
                w.ptr[1]->wpos = w.pos[1];
                if (w.next != nullptr) {
                    w.next->prev = nullptr;
                } else {
                    pending_.writer = nullptr;
                }
            } else {
                w.prev->eptr = w.ptr[1];
                w.prev->epos = w.pos[1];
                w.prev->next = w.next;
                if (w.next != nullptr) {
                    w.next->prev = w.prev;
                } else {
                    pending_.writer = w.prev;
                }
            }
        } while (0);
        if (w.prev == nullptr) {
            for (auto ptr = w.ptr[0]; ptr != w.ptr[1];) {
                auto next = ptr->next;
                ptr->wpos = N;
                ptr = next;
            }
        }
    }

    DataBuffer *head_, *tail_;
    DataPending pending_;
    spinlock spin_;
    std::atomic<size_t> size_;

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

    static ThreadSafePool<DataBuffer, S_MAX_SEND_BUFFER_POOL_COUNT(N)>
        buffer_pool_;
};

template <size_t N>
ThreadSafePool<
    typename TSendBuffer<N>::DataBuffer,
    S_MAX_SEND_BUFFER_POOL_COUNT(N)
> TSendBuffer<N>::buffer_pool_;

typedef TSendBuffer<65536> SendBuffer;
