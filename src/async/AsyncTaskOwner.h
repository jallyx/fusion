#pragma once

#include "ThreadSafeQueue.h"
#include "ThreadSafeSet.h"
#include "noncopyable.h"

class AsyncTask;

class AsyncTaskOwner : public noncopyable
{
public:
    AsyncTaskOwner();
    virtual ~AsyncTaskOwner();

    void Update();

    void AddTask(AsyncTask *task);
    void AddSubject(const void *subject);

    bool HasSubject() const;

private:
    ThreadSafeQueue<AsyncTask*> tasks_;
    ThreadSafeSet<const void *> subjects_;
};
