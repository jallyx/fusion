#include "ConnectionManager.h"
#include "IOServiceManager.h"
#include "Session.h"

ConnectionManager::ConnectionManager()
{
}

ConnectionManager::~ConnectionManager()
{
}

std::shared_ptr<Connection> ConnectionManager::NewConnection(Session &session)
{
    std::shared_ptr<Connection> connPtr =
        std::make_shared<Connection>(sIOServiceManager.SelectWorkerLoadLowest(),
            *this, session, session.GetConnectionLoadValue());
    connPtr->Init();
    AddConnection(connPtr);
    return connPtr;
}

void ConnectionManager::AddConnection(const std::shared_ptr<Connection> &connPtr)
{
    connections_.Insert(connPtr);
    sIOServiceManager.AddWorkerLoadValue(
        connPtr->get_io_service(), connPtr->get_load_value());
}

void ConnectionManager::RemoveConnection(const std::shared_ptr<Connection> &connPtr)
{
    connections_.Remove(connPtr);
    sIOServiceManager.SubWorkerLoadValue(
        connPtr->get_io_service(), connPtr->get_load_value());
}
