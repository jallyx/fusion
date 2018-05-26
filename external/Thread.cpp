#include "Thread.h"
#include "Exception.h"
#include "OS.h"

#if defined(_WIN32)
    #include "SetThreadName.h"
#endif

Thread::Thread()
: status_(Stopped)
, thread_running_(false)
{
}

Thread::~Thread()
{
}

bool Thread::Start()
{
    if (!IsStopped()) {
        return false;
    }

    if (!Prepare()) {
        return false;
    }

    Resume();

    try {
        thread_ = std::thread(std::bind(&Thread::Run, this));
        thread_running_ = true;
    } catch (...) {
        return false;
    }

    return true;
}

void Thread::Stop()
{
    if (IsStopped()) {
        return;
    }

    status_ = Stopped;

    if (thread_running_) {
        thread_running_ = false;
    } else {
        return;
    }

    if (thread_.get_id() == std::this_thread::get_id()) {
        thread_.detach();
        return;
    }

    Abort();
    thread_.join();
}

void Thread::Resume()
{
    status_ = Running;
}

void Thread::Pause()
{
    status_ = Paused;
}

void Thread::Run()
{
#if defined(_WIN32)
    SetThreadName(-1, GetThreadName());
#endif

    if (!Initialize()) {
        Stop();
        return;
    }

    while (status_ != Stopped) {
        if (status_ == Running) {
            TRY_BEGIN {
                Kernel();
            } TRY_END
            CATCH_BEGIN(const IException &e) {
                e.Print();
            } CATCH_END
            CATCH_BEGIN(...) {
            } CATCH_END
            continue;
        }
        if (status_ == Paused) {
            OS::SleepMS(100);
            continue;
        }
    }

    Finish();
}
