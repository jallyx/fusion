#include "AsyncTaskOwner.h"
#include "AsyncTask.h"
#include "Exception.h"

AsyncTaskOwner::AsyncTaskOwner()
: event_observer_(nullptr)
, is_overstocked_task_(false)
{
}

AsyncTaskOwner::~AsyncTaskOwner()
{
    AsyncTask *task = nullptr;
    while (tasks_.Dequeue(task)) {
        delete task;
    }
    if (event_observer_ != nullptr) {
        event_observer_->OnDeleteOwner(this);
    }
}

void AsyncTaskOwner::UpdateTask()
{
    AsyncTask *task = nullptr;
    while (tasks_.Dequeue(task)) {
        subjects_.Remove(task);
        TRY_BEGIN {
            task->Finish(this);
        } TRY_END
        CATCH_BEGIN(const IException &e) {
            e.Print();
        } CATCH_END
        CATCH_BEGIN(...) {
        } CATCH_END
        delete task;
    }
}

void AsyncTaskOwner::AddTask(AsyncTask *task)
{
    tasks_.Enqueue(task);
    if (!is_overstocked_task_ && event_observer_ != nullptr) {
        is_overstocked_task_ = true;
        event_observer_->OnAddTask(this);
    }
}

void AsyncTaskOwner::AddSubject(const void *subject)
{
    subjects_.Insert(subject);
}

bool AsyncTaskOwner::HasSubject() const
{
    return !subjects_.IsEmpty();
}

void AsyncTaskOwner::SetEventObserver(IEventObserver *observer)
{
    event_observer_ = observer;
}

void AsyncTaskOwner::ClearTaskOverstockFlag()
{
    is_overstocked_task_ = false;
}
