#pragma once

#include <mutex>
#include <queue>
#include "ThreadSafePool.h"

// be useful for:
// multi producer, single consumer.

template <typename T, size_t N>
class ThreadSafeDoubleQueue
{
private:
    class SwapBuffer {
    public:
        SwapBuffer() : rpos_(0), wpos_(0) {}
        bool IsEmpty() const { return rpos_ >= wpos_; }
        bool IsFull() const { return wpos_ >= N; }
        T Get() { return pool_[rpos_++]; }
        void Put(T v) { pool_[wpos_++] = v; }
        void Reset() { rpos_ = wpos_ = 0; }
    private:
        T pool_[N];
        size_t rpos_, wpos_;
    };

public:
    ThreadSafeDoubleQueue()
        : in_buffer_(&swap_buffer_[0])
        , out_buffer_(&swap_buffer_[1])
    {}
    ~ThreadSafeDoubleQueue() {
        for (; !in_buffers_.empty(); in_buffers_.pop()) {
            FreeBuffer(in_buffers_.front());
        }
        FreeBuffer(in_buffer_);
        FreeBuffer(out_buffer_);
    }

    void Enqueue(const T &v) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (in_buffer_->IsFull()) {
            in_buffers_.push(in_buffer_);
            in_buffer_ = AllocBuffer();
        }
        in_buffer_->Put(v);
    }

    bool Dequeue(T &v) {
        if (!out_buffer_->IsEmpty()) {
            v = out_buffer_->Get();
            return true;
        }
        return false;
    }

    bool Swap() {
        if (out_buffer_->IsEmpty()) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (in_buffers_.empty()) {
                std::swap(out_buffer_, in_buffer_);
                in_buffer_->Reset();
            } else {
                FreeBuffer(out_buffer_);
                out_buffer_ = in_buffers_.front();
                in_buffers_.pop();
            }
        }
        return !out_buffer_->IsEmpty();
    }

private:
    SwapBuffer *AllocBuffer() {
        SwapBuffer *buffer = idle_pool_.Get();
        return buffer != nullptr ? buffer : new SwapBuffer();
    }

    void FreeBuffer(SwapBuffer *buffer) {
        if (buffer == &swap_buffer_[0] || buffer == &swap_buffer_[1]) {
            buffer->Reset(), idle_pool_.Put(buffer);
        } else {
            delete buffer;
        }
    }

    std::mutex mutex_;
    std::queue<SwapBuffer*> in_buffers_;
    SwapBuffer *in_buffer_;
    SwapBuffer *out_buffer_;
    SwapBuffer swap_buffer_[2];
    ThreadSafePool<SwapBuffer, 2> idle_pool_;
};
