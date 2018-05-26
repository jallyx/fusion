#pragma once

#include "Singleton.h"
#include <unordered_set>
#include "Session.h"
#include "ThreadSafeQueue.h"

class SessionManager : public Singleton<SessionManager>
{
public:
    SessionManager();
    virtual ~SessionManager();

    void Update();
    void Tick();
    void Stop();

    void AddSession(Session *session);
    void KillSession(Session *session);
    void ShutdownSession(Session *session);

    void SetExternalCleanup(const std::function<void()> &func) {
        external_cleanup_ = func;
    }

private:
    void RemoveSession(Session *session);

    void CheckSessions();
    void UpdateSessions();
    void TickSessions();

    void ShutdownAll();

    std::unordered_set<Session*> sessions_;
    ThreadSafeQueue<Session*> waiting_room_;
    ThreadSafeQueue<Session*> recycle_bin_;

    std::function<void()> external_cleanup_;
};

#define sSessionManager (*SessionManager::instance())
