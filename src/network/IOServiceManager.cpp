#include "IOServiceManager.h"
#include "AsioWorkingThread.h"

IOServiceManager::IOServiceManager()
: worker_count_(1)
{
}

IOServiceManager::~IOServiceManager()
{
}

bool IOServiceManager::Prepare()
{
    for (size_t i = 0; i < worker_count_ || i < 1; ++i) {
        worker_load_.push_back(new std::atomic_int());
        io_service_.push_back(new boost::asio::io_service());
        io_work_.push_back(new boost::asio::io_service::work(*io_service_[i]));
        PushThread(new AsioWorkingThread(*io_service_[i]));
    }
    return true;
}

void IOServiceManager::Abort()
{
    for (auto io_work : io_work_) {
        delete io_work;
    }
    io_work_.clear();
}

void IOServiceManager::Finish()
{
    for (auto io_service : io_service_) {
        delete io_service;
    }
    io_service_.clear();
    for (auto worker_load : worker_load_) {
        delete worker_load;
    }
    worker_load_.clear();
}

boost::asio::io_service &IOServiceManager::SelectWorkerLoadLowest() const
{
    auto itr = std::min_element(worker_load_.begin(), worker_load_.end(),
            [](const std::atomic_int *p1, const std::atomic_int *p2) {
        return p1->load() < p2->load();
    });
    return *io_service_[itr - worker_load_.begin()];
}

void IOServiceManager::AddWorkerLoadValue(boost::asio::io_service &io_service, int value)
{
    auto itr = std::find(io_service_.begin(), io_service_.end(), &io_service);
    worker_load_[itr - io_service_.begin()]->fetch_add(value);
}

void IOServiceManager::SubWorkerLoadValue(boost::asio::io_service &io_service, int value)
{
    auto itr = std::find(io_service_.begin(), io_service_.end(), &io_service);
    worker_load_[itr - io_service_.begin()]->fetch_sub(value);
}
