#pragma once

#include "ThreadSafeQueue.h"
#include "ThreadSafeSet.h"
#include "noncopyable.h"
#include "enable_linked_from_this.h"

class AsyncTask;

class AsyncTaskOwner : public noncopyable,
    public enable_linked_from_this<AsyncTaskOwner>
{
public:
    class IEventObserver {
    public:
        virtual void OnAddTask(AsyncTaskOwner *owner) = 0;
        virtual void OnDeleteOwner(AsyncTaskOwner *owner) = 0;
    };

    AsyncTaskOwner();
    virtual ~AsyncTaskOwner();

    void SetEventObserver(IEventObserver *observer);
    void ClearTaskOverstockFlag();

    void UpdateTask();

    void AddTask(AsyncTask *task);
    void AddSubject(const void *subject);

    bool HasSubject() const;

private:
    ThreadSafeQueue<AsyncTask*> tasks_;
    ThreadSafeSet<const void *> subjects_;

    IEventObserver *event_observer_;
    bool is_overstocked_task_;
};
