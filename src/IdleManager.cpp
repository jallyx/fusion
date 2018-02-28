#include "IdleManager.h"
#include "System.h"

IdleManager::IdleManager()
{
}

IdleManager::~IdleManager()
{
}

void IdleManager::Run(long ms)
{
    const uint64 expired_time = GET_REAL_SYS_TIME + ms;
    while (true) {
        RunSession();
        RunAsyncTaskOwner();
        const uint64 current_time = GET_REAL_SYS_TIME;
        if (current_time >= expired_time) {
            break;
        }
        const std::chrono::milliseconds duration(expired_time - current_time);
        if (cv_.wait_for(fakelock_, duration) == std::cv_status::timeout) {
            break;
        }
    }
}

void IdleManager::RunAsyncTaskOwner()
{
    AsyncTaskOwner *owner = nullptr;
    while ((owner = async_task_owner_pool_.Get()) != nullptr || async_task_owner_queue_.Dequeue(owner)) {
        if (async_task_owner_discarded_list_.count(owner) == 0) {
            owner->ClearTaskOverstockFlag();
            owner->UpdateTask();
        }
    }
    async_task_owner_discarded_list_.clear();
}

void IdleManager::RunSession()
{
    Session *session = nullptr;
    while ((session = session_pool_.Get()) != nullptr || session_queue_.Dequeue(session)) {
        if (session_discarded_list_.count(session) == 0) {
            session->ClearPacketOverstockFlag();
            session->Update();
        }
    }
    session_discarded_list_.clear();
}

void IdleManager::OnAddTask(AsyncTaskOwner *owner)
{
    if (!async_task_owner_pool_.Put(owner)) {
        async_task_owner_queue_.Enqueue(owner);
    }
    cv_.notify_one();
}

void IdleManager::OnDeleteOwner(AsyncTaskOwner *owner)
{
    async_task_owner_discarded_list_.insert(owner);
}

void IdleManager::OnRecvPacket(Session *session)
{
    if (!session_pool_.Put(session)) {
        session_queue_.Enqueue(session);
    }
    cv_.notify_one();
}

void IdleManager::OnShutdownSession(Session *session)
{
    session_discarded_list_.insert(session);
}
