#pragma once

#include "Singleton.h"
#include "ThreadPool.h"
#include <atomic>
#include "AsioHeader.h"

class IOServiceManager : public ThreadPool, public Singleton<IOServiceManager>
{
public:
    IOServiceManager();
    virtual ~IOServiceManager();

    void SetWorkerCount(size_t count) { worker_count_ = count;}

    boost::asio::io_service &SelectWorkerLoadLowest() const;
    void AddWorkerLoadValue(boost::asio::io_service &io_service, int value);
    void SubWorkerLoadValue(boost::asio::io_service &io_service, int value);

private:
    virtual bool Prepare();
    virtual void Abort();
    virtual void Finish();

    size_t worker_count_;
    std::vector<std::atomic_int*> worker_load_;
    std::vector<boost::asio::io_service*> io_service_;
    std::vector<boost::asio::io_service::work*> io_work_;
};

#define sIOServiceManager (*IOServiceManager::instance())
