#pragma once

#include "Thread.h"
#include "ThreadSafeQueue.h"

class AsyncTask;
class AsyncTaskOwner;

class AsyncWorkingThread : public Thread
{
public:
    THREAD_RUNTIME(AsyncWorkingThread)

    AsyncWorkingThread(ThreadSafeQueue<std::pair<AsyncTask*, AsyncTaskOwner*>> &tasks);
    virtual ~AsyncWorkingThread();

protected:
    virtual void Kernel();

private:
    ThreadSafeQueue<std::pair<AsyncTask*, AsyncTaskOwner*>> &tasks_;
};
