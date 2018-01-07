#pragma once

#include <thread>
#include "noncopyable.h"

#define THREAD_RUNTIME(name) \
    virtual const char *GetThreadName() { return #name; }

class Thread : public noncopyable
{
public:
    THREAD_RUNTIME(Thread)

    enum Status {
        Stopped,
        Running,
        Paused,
    };

    Thread();
    virtual ~Thread();

    bool Start();
    void Stop();

    void Resume();
    void Pause();

    bool IsStopped() const { return status_ == Stopped; }
    bool IsRunning() const { return status_ == Running; }
    bool IsPaused() const { return status_ == Paused; }

protected:
    virtual bool Prepare() { return true; }
    virtual bool Initialize() { return true; }
    virtual void Kernel() = 0;
    virtual void Abort() {}
    virtual void Finish() {}

private:
    void Run();

    Status status_;
    std::thread thread_;
};
