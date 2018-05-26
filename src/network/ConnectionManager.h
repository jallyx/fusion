#pragma once

#include "Singleton.h"
#include <memory>
#include "Connection.h"
#include "ThreadSafeSet.h"

class ConnectionManager : public Singleton<ConnectionManager>
{
public:
    ConnectionManager();
    virtual ~ConnectionManager();

    std::shared_ptr<Connection> NewConnection(Session &session);
    void AddConnection(const std::shared_ptr<Connection> &connPtr);
    void RemoveConnection(const std::shared_ptr<Connection> &connPtr);

private:
    ThreadSafeSet<std::shared_ptr<Connection>> connections_;
};

#define sConnectionManager (*ConnectionManager::instance())
