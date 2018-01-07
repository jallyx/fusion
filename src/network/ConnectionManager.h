#pragma once

#include "ThreadPool.h"
#include <memory>
#include <mutex>
#include <unordered_set>
#include "Connection.h"

class ConnectionManager : public ThreadPool
{
public:
    ConnectionManager();
    virtual ~ConnectionManager();

    void SetWorkerCount(size_t count) { worker_count_ = count;}

    std::shared_ptr<Connection> NewConnection(Session &session);
    void AddConnection(const std::shared_ptr<Connection> &connPtr);
    void RemoveConnection(const std::shared_ptr<Connection> &connPtr);

    asio::io_service &io_service() { return io_service_; }

private:
    virtual bool Prepare();
    virtual void Abort();

    size_t worker_count_;
    asio::io_service io_service_;
    asio::io_service::work *io_work_;

    std::mutex mutex_;
    std::unordered_set<std::shared_ptr<Connection>> connections_;
};
