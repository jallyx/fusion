#include "ThreadPool.h"

ThreadPool::ThreadPool()
{
}

ThreadPool::~ThreadPool()
{
}

bool ThreadPool::Start()
{
    if (!Prepare()) {
        return false;
    }

    for (auto thread : threads_) {
        if (!thread->Start()) {
            return false;
        }
    }

    return true;
}

void ThreadPool::Stop()
{
    Abort();

    for (auto thread : threads_) {
        thread->Stop();
    }

    Finish();
    ClearThreads();
}

void ThreadPool::PushThread(Thread *thread)
{
    threads_.push_back(thread);
}

void ThreadPool::ClearThreads()
{
    for (auto thread : threads_) {
        delete thread;
    }
    threads_.clear();
}
