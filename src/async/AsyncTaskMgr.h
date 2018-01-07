#pragma once

#include "Singleton.h"
#include "ThreadPool.h"
#include "AsyncTask.h"
#include "AsyncTaskOwner.h"
#include "ThreadSafeQueue.h"
#include <condition_variable>

class AsyncTaskMgr : public ThreadPool, public Singleton<AsyncTaskMgr>
{
public:
    AsyncTaskMgr();
    virtual ~AsyncTaskMgr();

    bool HasTask() const;
    bool WaitTask(const void *queue);

    void AddTask(AsyncTask *task, AsyncTaskOwner *owner = nullptr);
    void AddHeavyTask(AsyncTask *task, AsyncTaskOwner *owner = nullptr);

protected:
    virtual bool Prepare();
    virtual void Finish();

private:
    size_t worker_count_;

    std::mutex mutex_, heavy_mutex_;
    std::condition_variable cv_, heavy_cv_;
    ThreadSafeQueue<std::pair<AsyncTask*, AsyncTaskOwner*>> tasks_;
    ThreadSafeQueue<std::pair<AsyncTask*, AsyncTaskOwner*>> heavy_tasks_;
};

#define sAsyncTaskMgr (*AsyncTaskMgr::instance())
