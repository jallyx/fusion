#include "AsyncWorkingThread.h"
#include "AsyncTask.h"
#include "AsyncTaskOwner.h"
#include "Exception.h"
#include "Macro.h"

AsyncWorkingThread::AsyncWorkingThread(
    MultiBufferQueue<std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>> &tasks)
: shared_tasks_(tasks)
, idle_(false)
{
}

AsyncWorkingThread::~AsyncWorkingThread()
{
}

void AsyncWorkingThread::AddTask(
    const std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>> &task)
{
    alone_tasks_.Enqueue(task);
    cv_.notify_one();
}

bool AsyncWorkingThread::WakeIdle()
{
    if (idle_) {
        cv_.notify_one();
        return true;
    } else {
        return false;
    }
}

bool AsyncWorkingThread::HasTask()
{
    return !alone_tasks_.IsEmpty();
}

void AsyncWorkingThread::Kernel()
{
    do {
        std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>> pair;
        while (alone_tasks_.Dequeue(pair) || shared_tasks_.DequeueSafe(pair)) {
            TRY_BEGIN {
                pair.first->ExecuteInAsync();
            } TRY_END
            CATCH_BEGIN(const IException &e) {
                e.Print();
            } CATCH_END
            CATCH_BEGIN(...) {
            } CATCH_END
            if (!pair.second.expired()) {
                pair.second.lock()->AddTask(pair.first);
            } else {
                delete pair.first;
            }
        }
    } while (WaitTask());
}

void AsyncWorkingThread::Finish()
{
    std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>> pair;
    while (alone_tasks_.Dequeue(pair)) {
        delete pair.first;
    }
}

bool AsyncWorkingThread::WaitTask()
{
    idle_ = true;
    _defer(idle_ = false);
    const std::chrono::milliseconds duration(100);
    return cv_.wait_for(fakelock_, duration) == std::cv_status::no_timeout;
}
