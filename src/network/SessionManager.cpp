#include "SessionManager.h"
#include "OS.h"

SessionManager::SessionManager()
{
}

SessionManager::~SessionManager()
{
    ClearAll();
}

void SessionManager::Update()
{
    CheckSessions();
    UpdateSessions();
}

void SessionManager::Tick()
{
    TickSessions();
}

void SessionManager::Stop()
{
    ShutdownAll();
    ClearAll();
}

void SessionManager::CheckSessions()
{
    Session *session = nullptr;
    while (waiting_room_.Dequeue(session)) {
        if (session->IsActive()) {
            session->OnManaged();
            session->SetStatus(Session::Running);
        }
        sessions_.insert(session);
    }

    const size_t size = recycle_bin_.GetSize();
    for (size_t i = 0; i < size && recycle_bin_.Dequeue(session); ++i) {
        if (sessions_.erase(session) != 0) {
            session->OnShutdownSession();
        }
        if (session->IsIndependent()) {
            session->DeleteObject();
            continue;
        }
        if (!session->HasSendDataAwaiting() || session->IsShutdownExpired()) {
            session->Disconnect();
        }
        recycle_bin_.Enqueue(session);
    }
}

void SessionManager::UpdateSessions()
{
    for (auto session : sessions_) {
        session->Update();
    }
}

void SessionManager::TickSessions()
{
    for (auto session : sessions_) {
        session->CheckTimeout();
    }
}

void SessionManager::AddSession(Session *session)
{
    waiting_room_.Enqueue(session);
    session->SetManager(this);
}

void SessionManager::RemoveSession(Session *session)
{
    recycle_bin_.Enqueue(session);
    session->SetManager(nullptr);
}

void SessionManager::KillSession(Session *session)
{
    if (session->IsActive()) {
        session->Disconnect();
        session->ShutdownSession();
    }
}

void SessionManager::ShutdownSession(Session *session)
{
    if (session->IsActive()) {
        session->Disable();
        if (session->GrabShutdownFlag()) {
            RemoveSession(session);
        }
    }
}

void SessionManager::ShutdownAll()
{
    for (auto session : sessions_) {
        session->ShutdownSession();
    }
}

void SessionManager::ClearAll()
{
    auto SafeSessionDeleter = [](Session *session) {
        session->Disconnect();
        while (!session->IsIndependent())
            OS::SleepMS(1);
        session->DeleteObject();
    };

    Session *session = nullptr;
    while (waiting_room_.Dequeue(session)) {
        SafeSessionDeleter(session);
    }
    while (recycle_bin_.Dequeue(session)) {
        SafeSessionDeleter(session);
        sessions_.erase(session);
    }
    for (auto session : sessions_) {
        SafeSessionDeleter(session);
    }
    sessions_.clear();
}
