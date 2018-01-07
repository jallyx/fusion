#pragma once

#include <vector>
#include "Thread.h"
#include "noncopyable.h"

class ThreadPool : public noncopyable
{
public:
    ThreadPool();
    virtual ~ThreadPool();

    bool Start();
    void Stop();

protected:
    virtual bool Prepare() = 0;
    virtual void Abort() {}
    virtual void Finish() {}

    void PushThread(Thread *thread);

private:
    void ClearThreads();

    std::vector<Thread*> threads_;
};
