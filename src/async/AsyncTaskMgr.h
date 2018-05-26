#pragma once

#include "Singleton.h"
#include "ThreadPool.h"
#include "AsyncTask.h"
#include "AsyncTaskOwner.h"
#include "ThreadSafeQueue.h"
#include <condition_variable>
#include "Concurrency.h"

class AsyncTaskMgr : public ThreadPool, public Singleton<AsyncTaskMgr>
{
public:
    AsyncTaskMgr();
    virtual ~AsyncTaskMgr();

    bool HasTask() const;
    bool WaitTask(const void *queue);

    void AddTask(AsyncTask *task, AsyncTaskOwner *owner = nullptr);
    void AddHeavyTask(AsyncTask *task, AsyncTaskOwner *owner = nullptr);

    void SetWorkerCount(size_t count) { worker_count_ = count; }

protected:
    virtual bool Prepare();
    virtual void Finish();

private:
    static std::weak_ptr<AsyncTaskOwner> null_owner_;
    size_t worker_count_;

    fakelock fakelock_, heavy_fakelock_;
    std::condition_variable_any cv_, heavy_cv_;
    ThreadSafeQueue<std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>> tasks_;
    ThreadSafeQueue<std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>> heavy_tasks_;
};

#define sAsyncTaskMgr (*AsyncTaskMgr::instance())
