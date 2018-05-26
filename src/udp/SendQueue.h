#pragma once

#include "Base.h"
#include "Macro.h"
#include "Debugger.h"
#include "InlineFuncs.h"

// Thread Safety
// Distinct objects : Safe.
// Shared objects : Unsafe.

// Flag(u8)|Age(u8)|Stamp(u32)|Prev(*&u32)|Next(*&u32)|Size(u16)|SEQ(u64)|xxxx
#define PKT_LEN_FLAG (sizeof(u8))
#define PKT_LEN_AGE (sizeof(u8))
#define PKT_LEN_STAMP (sizeof(u32))
#define PKT_LEN_PREV (sizeof(void*)+sizeof(u32))
#define PKT_LEN_NEXT (sizeof(void*)+sizeof(u32))
#define PKT_LEN_SIZE (sizeof(u16))
#define PKT_LEN_SEQ (sizeof(u64))
#define PKT_OFF_FLAG (0)
#define PKT_OFF_AGE (PKT_OFF_FLAG+PKT_LEN_FLAG)
#define PKT_OFF_STAMP (PKT_OFF_AGE+PKT_LEN_AGE)
#define PKT_OFF_PREV (PKT_OFF_STAMP+PKT_LEN_STAMP)
#define PKT_OFF_NEXT (PKT_OFF_PREV+PKT_LEN_PREV)
#define PKT_OFF_SIZE (PKT_OFF_NEXT+PKT_LEN_NEXT)
#define PKT_OFF_SEQ (PKT_OFF_SIZE+PKT_LEN_SIZE)
#define PKT_INFO (PKT_OFF_SEQ+PKT_LEN_SEQ)
#define PKT_PTR (sizeof(void*))

#define UDP_PKT_MAX (1400)  // MTU limit.
#define UDP_SEND_BUFFER_MAX (65536)

#define MAX_SEND_QUEUE_POOL_MEMORY (8*1024*1024)
#define MAX_SEND_QUEUE_POOL_COUNT (4096)
#define S_MAX_SEND_QUEUE_POOL_COUNT(N) \
    MAX(MIN(MAX_SEND_QUEUE_POOL_MEMORY/N, MAX_SEND_QUEUE_POOL_COUNT), 1)

template <size_t N>
class TSendQueue
{
    struct DataBuffer {
        DataBuffer *next = nullptr;
        size_t rpos = 0, wpos = 0;
        char buffer[N];
    };

public:
    struct PktPtr {
        PktPtr(DataBuffer *ptr = nullptr, size_t pos = 0)
            : ptr(ptr), pos(pos) {}
        bool operator==(const PktPtr &other) const {
            return ptr == other.ptr && pos == other.pos;
        }
        bool operator!=(const PktPtr &other) const {
            return !operator==(other);
        }
        operator bool() const { return ptr != nullptr; }
        DataBuffer *ptr;
        size_t pos;
    };

    struct OnSentPktRst {
        uint32 stamp = 0;
        bool lost = false;
    };

    struct DataWriter {
        PktPtr prev;
        DataBuffer *ptr;
        size_t pos;
        size_t n;
    };

    class PktValue {
    public:
        PktValue(uint8 age = 0, uint64 seq = 0)
            : n_(0), age_(age), seq_(seq) {}
        struct cstr { const char *str; size_t len; };
        typedef const struct cstr *const_iterator;
        const_iterator begin() const { return buffs_; }
        const_iterator end() const { return buffs_ + n_; }
        const uint8 age() const { return age_; }
        const uint64 seq() const { return seq_; }
        bool empty() const { return n_ == 0; }
        void add(const char *str, size_t len) {
            buffs_[n_++] = cstr{ str, len };
            assert(n_ <= ARRAY_SIZE(buffs_));
        }
    private:
        cstr buffs_[UDP_PKT_MAX/N+2];
        size_t n_;
        uint8 age_;
        uint64 seq_;
    };

    TSendQueue() : head_(AllocBuffer()), size_(0), num_(0), seq_(0) {
        tail_ = head_;
    }
    ~TSendQueue() {
        do {
            auto next = head_->next;
            FreeBuffer(head_);
            head_ = next;
        } while (head_ != nullptr);
    }

    void Prepare(DataWriter &w) {
        w.ptr = tail_, w.pos = tail_->wpos, w.n = 0;
        if ((tail_->wpos += PKT_INFO) >= N) {
            tail_->next = AllocBuffer();
            tail_ = tail_->next;
            tail_->wpos = w.ptr->wpos - N;
            w.ptr->wpos = N;
        }
    }

    void IncrementWritten(DataWriter &w, size_t size) {
        DBGASSERT(tail_->wpos + size <= N);
        tail_->wpos += size, w.n += size;
        if (tail_->wpos >= N) {
            tail_->next = AllocBuffer();
            tail_ = tail_->next;
            tail_->wpos = 0;
        }
    }

    void Flush(DataWriter &w) {
        DBGASSERT(w.n != 0 && w.n <= UDP_PKT_MAX);
        PktPtr ptr{ w.ptr, w.pos };
        WriteValue(ptr, PKT_OFF_FLAG, u8(0));
        WriteValue(ptr, PKT_OFF_AGE, u8(0));
        WriteValue(ptr, PKT_OFF_STAMP, u32(0));
        WriteValue(ptr, PKT_OFF_SIZE, u16(w.n));
        WriteValue(ptr, PKT_OFF_SEQ, u64(seq_));
        if (w.prev) {
            PktPtr next = ReadPktPtr(w.prev, PKT_OFF_NEXT);
            WritePktPtr(ptr, PKT_OFF_PREV, w.prev);
            WritePktPtr(ptr, PKT_OFF_NEXT, next);
            WritePktPtr(w.prev, PKT_OFF_NEXT, ptr);
            if (next) {
                WritePktPtr(next, PKT_OFF_PREV, ptr);
            } else {
                last_ = ptr;
            }
        } else {
            WriteValue(ptr, PKT_OFF_PREV, nullptr);
            WritePktPtr(ptr, PKT_OFF_NEXT, first_);
            if (first_) {
                WritePktPtr(first_, PKT_OFF_PREV, ptr);
            } else {
                last_ = ptr;
            }
            first_ = ptr;
        }
        size_ += w.n, seq_ += w.n, num_ += 1;
        w.prev = ptr;
    }

    void Revert(DataWriter &w) {
        DBGASSERT(w.n == 0);
        tail_ = w.ptr, tail_->wpos = w.pos;
        if (tail_->next != nullptr) {
            FreeBuffer(tail_->next);
            tail_->next = nullptr;
        }
    }

    char *GetWritableSpace(size_t &size) const {
        size = N - tail_->wpos;
        return tail_->buffer + tail_->wpos;
    }

    uint32 GetFirstPktStamp() const {
        if (first_) {
            return ReadValue<u32>(first_, PKT_OFF_STAMP);
        } else {
            return 0;
        }
    }

    PktValue SendRtoPkt(uint32 stamp, uint32 expiry) {
        if (first_ && ReadValue<u32>(first_, PKT_OFF_STAMP) <= expiry) {
            return SendFirstPktData(stamp);
        } else {
            return{};
        }
    }

    void OnPktSent(uint8 age, uint64 seq, uint64 una, OnSentPktRst &rst) {
        PktPtr next;
        for (PktPtr ptr{first_}; ptr; ptr = next) {
            next = ReadPktPtr(ptr, PKT_OFF_NEXT);
            auto i = ReadValue<u64>(ptr, PKT_OFF_SEQ);
            if (i == seq || i < una) {
                if (i == seq && age == ReadValue<u8>(ptr, PKT_OFF_AGE)) {
                    rst.stamp = ReadValue<u32>(ptr, PKT_OFF_STAMP);
                    rst.lost = ptr != first_;
                }
                RemovePktSent(ptr);
                RemoveSendData(ptr);
            }
        }
    }

    bool IsFull() const { return size_ >= UDP_SEND_BUFFER_MAX; }

    size_t GetDataSize() const { return size_; }
    size_t GetPktNum() const { return num_; }

private:
    template <typename T>
    T ReadValue(const PktPtr &ptr, size_t skip) const {
        T v; Read({ptr.ptr,ptr.pos+skip}, (char*)&v, sizeof(v)); return v;
    }
    template <typename T>
    void WriteValue(const PktPtr &ptr, size_t skip, const T &v) {
        Write({ptr.ptr,ptr.pos+skip}, (const char*)&v, sizeof(v));
    }

    PktPtr ReadPktPtr(const PktPtr &ptr, size_t skip) const {
        PktPtr v;
        if ((v.ptr = ReadValue<DataBuffer*>(ptr, skip)) != nullptr) {
            v.pos = ReadValue<u32>(ptr, skip + PKT_PTR);
        }
        return v;
    }
    void WritePktPtr(const PktPtr &ptr, size_t skip, const PktPtr &v) {
        WriteValue(ptr, skip, v.ptr);
        if (v.ptr != nullptr) {
            WriteValue(ptr, skip + PKT_PTR, u32(v.pos));
        }
    }

    void Read(PktPtr ptr, char *data, size_t size) const {
        do {
            if (ptr.pos >= N) {
                ptr.pos -= N, ptr.ptr = ptr.ptr->next;
            }
            if (ptr.pos < N) {
                auto avail = std::min(N - ptr.pos, size);
                memcpy(data, ptr.ptr->buffer + ptr.pos, avail);
                ptr.pos += avail, data += avail, size -= avail;
            }
        } while (size > 0);
    }
    void Write(PktPtr ptr, const char *data, size_t size) {
        do {
            if (ptr.pos >= N) {
                ptr.pos -= N, ptr.ptr = ptr.ptr->next;
            }
            if (ptr.pos < N) {
                auto avail = std::min(N - ptr.pos, size);
                memcpy(ptr.ptr->buffer + ptr.pos, data, avail);
                ptr.pos += avail, data += avail, size -= avail;
            }
        } while (size > 0);
    }

    PktValue SendFirstPktData(uint32 stamp) {
        DataBuffer *ptr = first_.ptr;
        size_t pos = first_.pos, size = PKT_INFO;
        do {
            size_t avail = std::min(N - pos, size);
            pos += avail, size -= avail;
            if (pos >= N) {
                pos = 0, ptr = ptr->next;
            }
        } while (size > 0);
        PktValue pkt(ReadValue<u8>(first_, PKT_OFF_AGE) + 1,
            ReadValue<u64>(first_, PKT_OFF_SEQ));
        size = ReadValue<u16>(first_, PKT_OFF_SIZE);
        do {
            size_t avail = std::min(N - pos, size);
            pkt.add(ptr->buffer + pos, avail);
            pos += avail, size -= avail;
            if (pos >= N) {
                pos = 0, ptr = ptr->next;
            }
        } while (size > 0);
        WriteValue(first_, PKT_OFF_AGE, pkt.age());
        WriteValue(first_, PKT_OFF_STAMP, u32(stamp));
        if (first_ != last_) {
            PktPtr next = ReadPktPtr(first_, PKT_OFF_NEXT);
            WriteValue(next, PKT_OFF_PREV, nullptr);
            WriteValue(first_, PKT_OFF_NEXT, nullptr);
            WritePktPtr(last_, PKT_OFF_NEXT, first_);
            WritePktPtr(first_, PKT_OFF_PREV, last_);
            last_ = first_, first_ = next;
        }
        return pkt;
    }

    void RemovePktSent(const PktPtr &ptr) {
        WriteValue(ptr, PKT_OFF_FLAG, u8(1));
        PktPtr prev = ReadPktPtr(ptr, PKT_OFF_PREV);
        PktPtr next = ReadPktPtr(ptr, PKT_OFF_NEXT);
        if (prev) {
            WritePktPtr(prev, PKT_OFF_NEXT, next);
        }
        if (next) {
            WritePktPtr(next, PKT_OFF_PREV, prev);
        }
        if (first_ == ptr) {
            first_ = next;
        }
        if (last_ == ptr) {
            last_ = prev;
        }
        num_ -= 1;
    }

    void RemoveSendData(PktPtr ptr) {
        if (ptr.ptr == head_ && ptr.pos == head_->rpos) {
            while (ReadValue<u8>(ptr, PKT_OFF_FLAG) != 0) {
                size_t size = ReadValue<u16>(ptr, PKT_OFF_SIZE);
                size_ -= size, size += PKT_INFO;
                do {
                    size_t avail = std::min(N - head_->rpos, size);
                    head_->rpos += avail, size -= avail;
                    if (head_->rpos >= N) {
                        auto next = head_->next;
                        FreeBuffer(head_);
                        head_ = next;
                    }
                } while (size > 0);
                if (head_ != tail_ || head_->rpos < tail_->wpos) {
                    ptr.ptr = head_, ptr.pos = head_->rpos;
                } else {
                    break;
                }
            }
        }
    }

    DataBuffer *head_, *tail_;
    PktPtr first_, last_;
    size_t size_, num_;
    uint64 seq_;

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

    static ThreadSafePool<DataBuffer, S_MAX_SEND_QUEUE_POOL_COUNT(N)>
        buffer_pool_;
};

template <size_t N>
ThreadSafePool<
    typename TSendQueue<N>::DataBuffer,
    S_MAX_SEND_QUEUE_POOL_COUNT(N)
> TSendQueue<N>::buffer_pool_;

typedef TSendQueue<65536> SendQueue;
