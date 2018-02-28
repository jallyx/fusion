#pragma once

#include <unordered_set>
#include "Session.h"
#include "ThreadSafeQueue.h"

class SessionManager
{
public:
    SessionManager();
    ~SessionManager();

    void Update();
    void Tick();
    void Stop();

    void AddSession(Session *session);
    void KillSession(Session *session);
    void ShutdownSession(Session *session);

private:
    void RemoveSession(Session *session);

    void CheckSessions();
    void UpdateSessions();
    void TickSessions();

    void ShutdownAll();
    void ClearAll();

    std::unordered_set<Session*> sessions_;
    ThreadSafeQueue<Session*> waiting_room_;
    ThreadSafeQueue<Session*> recycle_bin_;
};
