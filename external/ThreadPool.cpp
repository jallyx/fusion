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

void ThreadPool::Resume()
{
    for (auto thread : threads_) {
        thread->Resume();
    }
}

void ThreadPool::Pause()
{
    for (auto thread : threads_) {
        thread->Pause();
    }
}

void ThreadPool::Foreach(const std::function<void(Thread*)> &func) const
{
    for (auto thread : threads_) {
        func(thread);
    }
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

Thread *ThreadPool::GetThreadInstance(size_t index) const
{
    return index < threads_.size() ? threads_[index] : nullptr;
}

size_t ThreadPool::GetThreadNumber() const
{
    return threads_.size();
}
