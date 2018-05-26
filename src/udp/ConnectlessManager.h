#pragma once

#include "Singleton.h"
#include <memory>
#include <unordered_map>
#include "Connectless.h"

class ConnectlessManager : public Singleton<ConnectlessManager>
{
public:
    ConnectlessManager();
    virtual ~ConnectlessManager();

    std::shared_ptr<Connectless> FindConnectless(const std::string &key) const;

    std::shared_ptr<Connectless> NewConnectless(
        const std::string &key, Sessionless &sessionless);
    void AddConnectless(
        const std::string &key, const std::shared_ptr<Connectless> &connPtr);
    void RemoveConnectless(const std::shared_ptr<Connectless> &connPtr);

    static std::string NewUniqueKey();

private:
    mutable rwlock rwlock_;
    std::unordered_map<std::string, std::shared_ptr<Connectless>> connectlesses_;
};

#define sConnectlessManager (*ConnectlessManager::instance())
