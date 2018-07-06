#pragma once

#include "Thread.h"
#include "ThreadSafeQueue.h"
#include <condition_variable>
#include "Concurrency.h"

class AsyncTask;
class AsyncTaskOwner;

class AsyncWorkingThread : public Thread
{
public:
    THREAD_RUNTIME(AsyncWorkingThread)

    AsyncWorkingThread(
        ThreadSafeQueue<std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>> &tasks);
    virtual ~AsyncWorkingThread();

    void AddTask(const std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>& task);

    bool WakeIdle();

    bool HasTask() const;

protected:
    virtual void Kernel();
    virtual void Finish();

private:
    bool WaitTask();

    ThreadSafeQueue<std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>> &shared_tasks_;
    ThreadSafeQueue<std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>> alone_tasks_;
    std::condition_variable_any cv_;
    fakelock fakelock_;
    bool idle_;
};
