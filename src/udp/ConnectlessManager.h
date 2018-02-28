#pragma once

#include "Thread.h"
#include <list>
#include "Connectless.h"
#include "ThreadSafeMap.h"
#include "ThreadSafeQueue.h"

class ConnectlessManager : public Thread
{
public:
    THREAD_RUNTIME(ConnectlessManager)

    ConnectlessManager();
    virtual ~ConnectlessManager();

    std::shared_ptr<Connectless> FindConnectless(const std::string& key) const;

    std::shared_ptr<Connectless> NewConnectless(const std::string& key,
        asio::io_service &io_service, Sessionless &sessionless);
    void AddConnectless(
        const std::string& key, const std::shared_ptr<Connectless> &conn);
    void RemoveConnectless(const std::string &key);

    void RefreshConnectless(const std::string &key, uint32 stamp, uint32 rto);

    static std::string NewUniqueKey();

private:
    virtual void Kernel();

    void Merge();
    void OnRto();

    struct ConnInfo {
        uint32 stamp, rto;
        std::shared_ptr<Connectless> conn;
        size_t slot;
        std::list<std::weak_ptr<ConnInfo>>::iterator itr;
    };

    static void PostRotRequest(const std::list<std::weak_ptr<ConnInfo>> &objs);

    ThreadSafeMap<std::string, std::shared_ptr<ConnInfo>> avail_conns_;

    ThreadSafeQueue<std::shared_ptr<ConnInfo>> chan_rto_objs_;

    std::vector<std::list<std::weak_ptr<ConnInfo>>> rto_objs_;
    uint32 tick_count_;
};
