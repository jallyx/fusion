#include "ConnectionManager.h"
#include "AsioWorkingThread.h"

ConnectionManager::ConnectionManager()
: worker_count_(1)
, io_work_(nullptr)
{
}

ConnectionManager::~ConnectionManager()
{
}

bool ConnectionManager::Prepare()
{
    io_work_ = new asio::io_service::work(io_service_);

    for (size_t i = 0; i < worker_count_ || i < 1; ++i) {
        PushThread(new AsioWorkingThread(io_service_));
    }

    return true;
}

void ConnectionManager::Abort()
{
    if (io_work_ != nullptr) {
        delete io_work_;
        io_work_ = nullptr;
    }
}

std::shared_ptr<Connection> ConnectionManager::NewConnection(Session &session)
{
    std::shared_ptr<Connection> connPtr =
        std::make_shared<Connection>(*this, session);
    connPtr->Init();
    AddConnection(connPtr);
    return connPtr;
}

void ConnectionManager::AddConnection(const std::shared_ptr<Connection> &connPtr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.insert(connPtr);
}

void ConnectionManager::RemoveConnection(const std::shared_ptr<Connection> &connPtr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.erase(connPtr);
}
