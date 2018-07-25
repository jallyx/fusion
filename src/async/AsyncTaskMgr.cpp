#include "AsyncTaskMgr.h"
#include "AsyncWorkingThread.h"

#define GetAsyncWorkingThread(i) \
    static_cast<AsyncWorkingThread*>(GetThreadInstance(i))

std::weak_ptr<AsyncTaskOwner> AsyncTaskMgr::null_owner_;

AsyncTaskMgr::AsyncTaskMgr()
: worker_count_(2)
{
}

AsyncTaskMgr::~AsyncTaskMgr()
{
}

bool AsyncTaskMgr::Prepare()
{
    for (size_t i = 0; i < worker_count_; ++i) {
        PushThread(new AsyncWorkingThread(shared_tasks_));
    }
    return true;
}

void AsyncTaskMgr::Finish()
{
    std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>> pair;
    while (shared_tasks_.Dequeue(pair)) {
        delete pair.first;
    }
}

bool AsyncTaskMgr::HasTask()
{
    if (!shared_tasks_.IsEmpty()) return true;
    for (size_t i = 0, n = GetThreadNumber(); i < n; ++i) {
        if (GetAsyncWorkingThread(i)->HasTask()) {
            return true;
        }
    }
    return false;
}

void AsyncTaskMgr::AddTask(AsyncTask *task, AsyncTaskOwner *owner, ssize_t group)
{
    auto pair = BindAsyncTaskOwner(task, owner);
    const size_t n = GetThreadNumber();
    if (n == 0) {
        shared_tasks_.Enqueue(pair);
    } else if (group == -1) {
        shared_tasks_.Enqueue(pair);
        WakeIdleAsyncWorkingThread();
    } else {
        const size_t i = n >= 2 ? group % (n - 1) + 1 : 0;
        GetAsyncWorkingThread(i)->AddTask(pair);
    }
}

void AsyncTaskMgr::AddHeavyTask(AsyncTask *task, AsyncTaskOwner *owner)
{
    auto pair = BindAsyncTaskOwner(task, owner);
    const size_t n = GetThreadNumber();
    if (n == 0) {
        shared_tasks_.Enqueue(pair);
    } else {
        GetAsyncWorkingThread(0)->AddTask(pair);
    }
}

void AsyncTaskMgr::WakeIdleAsyncWorkingThread()
{
    for (size_t i = 0, n = GetThreadNumber(); i < n; ++i) {
        if (GetAsyncWorkingThread(i)->WakeIdle()) {
            break;
        }
    }
}

auto AsyncTaskMgr::BindAsyncTaskOwner(AsyncTask *task, AsyncTaskOwner *owner) ->
    std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>
{
    std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>> pair(task, null_owner_);
    if (owner != nullptr) {
        owner->AddSubject(task);
        pair.second = owner->linked_from_this();
    }
    return pair;
}
