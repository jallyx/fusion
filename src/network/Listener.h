#pragma once

#include "Thread.h"
#include "Macro.h"

class ConnectionManager;
class SessionManager;
class Session;

class Listener : public Thread
{
public:
    THREAD_RUNTIME(Listener)

    Listener();
    virtual ~Listener();

protected:
    virtual bool Prepare();
    virtual bool Initialize();
    virtual void Kernel();
    virtual void Finish();

    virtual ConnectionManager *GetConnectionManager() = 0;
    virtual SessionManager *GetSessionManager() = 0;
    virtual std::string GetBindAddress() = 0;
    virtual std::string GetBindPort() = 0;

    virtual Session *NewSessionObject() = 0;

private:
    void OnAcceptComplete(int family, SOCKET sockfd);

    SOCKET sockfd_;

    ConnectionManager *connection_manager_;
    SessionManager *session_manager_;
    std::string addr_;
    std::string port_;
};
