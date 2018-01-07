#include "AsyncTaskMgr.h"
#include "AsyncWorkingThread.h"

AsyncTaskMgr::AsyncTaskMgr()
: worker_count_(2)
{
}

AsyncTaskMgr::~AsyncTaskMgr()
{
}

bool AsyncTaskMgr::Prepare()
{
    PushThread(new AsyncWorkingThread(heavy_tasks_));
    for (size_t i = 1; i < worker_count_ || i < 2; ++i) {
        PushThread(new AsyncWorkingThread(tasks_));
    }
    return true;
}

void AsyncTaskMgr::Finish()
{
    std::pair<AsyncTask*, AsyncTaskOwner*> pair;
    while (tasks_.Dequeue(pair)) {
        delete pair.first;
    }
    while (heavy_tasks_.Dequeue(pair)) {
        delete pair.first;
    }
}

bool AsyncTaskMgr::HasTask() const
{
    return !tasks_.IsEmpty() || !heavy_tasks_.IsEmpty();
}

bool AsyncTaskMgr::WaitTask(const void *queue)
{
    std::cv_status status;
    const std::chrono::milliseconds duration(100);
    if (&tasks_ == queue) {
        std::unique_lock<std::mutex> lock(mutex_);
        status = cv_.wait_for(lock, duration);
    } else if (&heavy_tasks_ == queue) {
        std::unique_lock<std::mutex> lock(heavy_mutex_);
        status = heavy_cv_.wait_for(lock, duration);
    } else {
        assert(false && "can't reached here.");
    }
    return status == std::cv_status::no_timeout;
}

void AsyncTaskMgr::AddTask(AsyncTask *task, AsyncTaskOwner *owner)
{
    if (owner != nullptr) owner->AddSubject(task);
    tasks_.Enqueue(std::make_pair(task, owner));
    cv_.notify_one();
}

void AsyncTaskMgr::AddHeavyTask(AsyncTask *task, AsyncTaskOwner *owner)
{
    if (owner != nullptr) owner->AddSubject(task);
    heavy_tasks_.Enqueue(std::make_pair(task, owner));
    heavy_cv_.notify_one();
}
