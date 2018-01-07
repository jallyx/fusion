#include "AsyncTaskOwner.h"
#include "AsyncTask.h"
#include "Exception.h"

AsyncTaskOwner::AsyncTaskOwner()
{
}

AsyncTaskOwner::~AsyncTaskOwner()
{
    AsyncTask *task = nullptr;
    while (tasks_.Dequeue(task)) {
        delete task;
    }
}

void AsyncTaskOwner::Update()
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
}

void AsyncTaskOwner::AddSubject(const void *subject)
{
    subjects_.Insert(subject);
}

bool AsyncTaskOwner::HasSubject() const
{
    return !subjects_.IsEmpty();
}
