#pragma once

#include <functional>
#include <vector>
#include "Thread.h"

class ThreadPool : public noncopyable
{
public:
    ThreadPool();
    virtual ~ThreadPool();

    bool Start();
    void Stop();

    void Resume();
    void Pause();

    void Foreach(const std::function<void(Thread*)> &func) const;

protected:
    virtual bool Prepare() = 0;
    virtual void Abort() {}
    virtual void Finish() {}

    void PushThread(Thread *thread);

    Thread *GetThreadInstance(size_t index) const;
    size_t GetThreadNumber() const;

private:
    void ClearThreads();

    std::vector<Thread*> threads_;
};
