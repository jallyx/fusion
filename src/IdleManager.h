#pragma once

#include <condition_variable>
#include "async/AsyncTaskOwner.h"
#include "network/Session.h"

class IdleManager : public AsyncTaskOwner::IEventObserver, public Session::IEventObserver
{
public:
    IdleManager();
    ~IdleManager();

    void Run(long ms);

    virtual void OnAddTask(AsyncTaskOwner *owner);
    virtual void OnDeleteOwner(AsyncTaskOwner *owner);

    virtual void OnRecvPacket(Session *session);
    virtual void OnShutdownSession(Session *session);

private:
    void RunAsyncTaskOwner();
    void RunSession();

    ThreadSafePool<AsyncTaskOwner, 256> async_task_owner_pool_;
    MultiBufferQueue<AsyncTaskOwner*, 256> async_task_owner_queue_;
    std::unordered_set<AsyncTaskOwner*> async_task_owner_discarded_list_;

    ThreadSafePool<Session, 1024> session_pool_;
    MultiBufferQueue<Session*, 1024> session_queue_;
    std::unordered_set<Session*> session_discarded_list_;

    fakelock fakelock_;
    std::condition_variable_any cv_;
};
