#pragma once

#include <cstdlib>
#include "InlineFuncs.h"
#include "ThreadSafePool.h"

// be useful for:
// multi producer, single or multi consumer.

#define S_MAX_BUFFER_QUEUE_POOL_COUNT (8)

template <typename T, size_t N = 128>
class MultiBufferQueue
{
    struct DataQueue {
        DataQueue *next = nullptr;
        size_t rpos = 0, wpos = 0;
        T queue[N];
    };

public:
    MultiBufferQueue() {
        std::call_once(s_queue_flag_, AutoQueuePool);
        tail_ = head_ = AllocQueue();
    }
    ~MultiBufferQueue() {
        do {
            auto next = head_->next;
            FreeQueue(head_);
            head_ = next;
        } while (head_ != nullptr);
    }

    bool IsEmpty() {
        std::lock_guard<spinlock> lock(w_spin_);
        return tail_->rpos >= tail_->wpos;
    }

    void Enqueue(const T &v) {
        DataQueue *queue = nullptr;
mark:   bool isOk = false;
        do {
            std::lock_guard<spinlock> lock(w_spin_);
            if (tail_->wpos >= N && queue != nullptr) {
                tail_ = tail_->next = queue;
                queue = nullptr;
            }
            if (tail_->wpos < N) {
                tail_->queue[tail_->wpos] = v;
                tail_->wpos += 1;
                isOk = true;
            }
        } while (0);
        if (isOk) {
            if (queue != nullptr) {
                FreeQueue(queue);
            }
        } else {
            queue = AllocQueue();
            goto mark;
        }
    }

    bool Dequeue(T &v) {
        if (head_->rpos < head_->wpos) {
mark:       v = head_->queue[head_->rpos++];
            return true;
        }
        if (head_->rpos >= N && head_->next != nullptr) {
            auto next = head_->next;
            FreeQueue(head_);
            head_ = next;
            goto mark;
        }
        return false;
    }

    bool DequeueSafe(T &v) {
        DataQueue *queue = nullptr;
        bool isOK = false;
        do {
            std::lock_guard<spinlock> lock(r_spin_);
            if (head_->rpos < head_->wpos) {
mark:           v = head_->queue[head_->rpos++];
                isOK = true;
                break;
            }
            if (head_->rpos >= N && head_->next != nullptr) {
                queue = head_, head_ = head_->next;
                goto mark;
            }
        } while (0);
        if (queue != nullptr) {
            FreeQueue(queue);
        }
        return isOK;
    }

private:
    DataQueue *head_, *tail_;
    spinlock r_spin_, w_spin_;

private:
    static void AutoQueuePool() {
        InitQueuePool();
        std::atexit(ClearQueuePool);
    }

    static void InitQueuePool() {
    }
    static void ClearQueuePool() {
        DataQueue *queue = nullptr;
        while ((queue = s_queue_pool_.Get()) != nullptr) {
            delete queue;
        }
    }

    static DataQueue *AllocQueue() {
        DataQueue *queue = nullptr;
        if ((queue = s_queue_pool_.Get()) != nullptr) {
            return REINIT_OBJECT(queue);
        } else {
            return new DataQueue;
        }
    }
    static void FreeQueue(DataQueue *queue) {
        if (!s_queue_pool_.Put(queue)) {
            delete queue;
        }
    }

    static std::once_flag s_queue_flag_;
    static ThreadSafePool<DataQueue, S_MAX_BUFFER_QUEUE_POOL_COUNT>
        s_queue_pool_;
};

template <typename T, size_t N>
std::once_flag MultiBufferQueue<T, N>::s_queue_flag_;
template <typename T, size_t N>
ThreadSafePool<
    typename MultiBufferQueue<T, N>::DataQueue,
    S_MAX_BUFFER_QUEUE_POOL_COUNT
> MultiBufferQueue<T, N>::s_queue_pool_;
