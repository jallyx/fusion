#pragma once

#include "Singleton.h"
#include "ThreadPool.h"
#include "AsyncTask.h"
#include "AsyncTaskOwner.h"

class AsyncTaskMgr : public ThreadPool, public Singleton<AsyncTaskMgr>
{
public:
    AsyncTaskMgr();
    virtual ~AsyncTaskMgr();

    bool HasTask();

    void AddTask(AsyncTask *task, AsyncTaskOwner *owner = nullptr, ssize_t group = -1);
    void AddHeavyTask(AsyncTask *task, AsyncTaskOwner *owner = nullptr);

    void SetWorkerCount(size_t count) { worker_count_ = count; }

protected:
    virtual bool Prepare();
    virtual void Finish();

private:
    void WakeIdleAsyncWorkingThread();

    static auto BindAsyncTaskOwner(AsyncTask *task, AsyncTaskOwner *owner) ->
        std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>;

    static std::weak_ptr<AsyncTaskOwner> null_owner_;
    size_t worker_count_;

    MultiBufferQueue<std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>> shared_tasks_;
};

#define sAsyncTaskMgr (*AsyncTaskMgr::instance())
