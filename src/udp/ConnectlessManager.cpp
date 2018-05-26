#include "ConnectlessManager.h"
#include "network/IOServiceManager.h"
#include "Sessionless.h"

ConnectlessManager::ConnectlessManager()
{
}

ConnectlessManager::~ConnectlessManager()
{
}

std::shared_ptr<Connectless> ConnectlessManager::FindConnectless(const std::string &key) const
{
    rwlock_.rdlock();
    std::lock_guard<rwlock> lock(rwlock_, std::adopt_lock);
    auto itr = connectlesses_.find(key);
    return itr != connectlesses_.end() ? itr->second : nullptr;
}

std::shared_ptr<Connectless> ConnectlessManager::NewConnectless(
        const std::string &key, Sessionless &sessionless)
{
    std::shared_ptr<Connectless> connPtr =
        std::make_shared<Connectless>(sIOServiceManager.SelectWorkerLoadLowest(),
            *this, sessionless, sessionless.GetConnectlessLoadValue());
    connPtr->Init();
    AddConnectless(key, connPtr);
    return connPtr;
}

void ConnectlessManager::AddConnectless(
        const std::string &key, const std::shared_ptr<Connectless> &connPtr)
{
    do {
        rwlock_.wrlock();
        std::lock_guard<rwlock> lock(rwlock_, std::adopt_lock);
        connectlesses_.emplace(key, connPtr);
    } while (0);
    sIOServiceManager.AddWorkerLoadValue(
        connPtr->get_io_service(), connPtr->get_load_value());
}

void ConnectlessManager::RemoveConnectless(const std::shared_ptr<Connectless> &connPtr)
{
    do {
        rwlock_.wrlock();
        std::lock_guard<rwlock> lock(rwlock_, std::adopt_lock);
        connectlesses_.erase(connPtr->key());
    } while (0);
    sIOServiceManager.SubWorkerLoadValue(
        connPtr->get_io_service(), connPtr->get_load_value());
}

std::string ConnectlessManager::NewUniqueKey()
{
    static std::atomic<uint32> counter{0};
    auto key = counter.fetch_add(1);
    return{(const char *)&key, sizeof(key)};
}
