#include "ConnectlessManager.h"
#include "System.h"
#include "OS.h"

static const size_t capacity = 8192;

ConnectlessManager::ConnectlessManager()
: tick_count_(u32(GET_REAL_APP_TIME))
{
    rto_objs_.resize(capacity + 1);
}

ConnectlessManager::~ConnectlessManager()
{
}

std::shared_ptr<Connectless> ConnectlessManager::FindConnectless(const std::string& key) const
{
    std::shared_ptr<ConnInfo> infoPtr;
    return avail_conns_.Get(key, infoPtr) ? infoPtr->conn : nullptr;
}

std::shared_ptr<Connectless> ConnectlessManager::NewConnectless(const std::string& key,
        asio::io_service &io_service, Sessionless &sessionless)
{
    std::shared_ptr<Connectless> connPtr =
        std::make_shared<Connectless>(io_service, *this, sessionless);
    connPtr->Init();
    AddConnectless(key, connPtr);
    return connPtr;
}

void ConnectlessManager::AddConnectless(
        const std::string& key, const std::shared_ptr<Connectless> &conn)
{
    std::shared_ptr<ConnInfo> infoPtr = std::make_shared<ConnInfo>();
    infoPtr->conn = conn, infoPtr->slot = capacity + 1;
    if (avail_conns_.Insert(key, infoPtr)) {
        chan_rto_objs_.Enqueue(infoPtr);
    }
}

void ConnectlessManager::RemoveConnectless(const std::string &key)
{
    std::shared_ptr<ConnInfo> infoPtr;
    if (avail_conns_.Remove(key, infoPtr)) {
        infoPtr->rto = 0;
        chan_rto_objs_.Enqueue(infoPtr);
    }
}

void ConnectlessManager::RefreshConnectless(const std::string &key, uint32 stamp, uint32 rto)
{
    DBGASSERT(stamp == 0 || stamp <= GET_REAL_APP_TIME);
    DBGASSERT(rto != 0 && rto < capacity);
    std::shared_ptr<ConnInfo> infoPtr;
    if (avail_conns_.Get(key, infoPtr)) {
        infoPtr->stamp = stamp;
        infoPtr->rto = rto;
        chan_rto_objs_.Enqueue(infoPtr);
    }
}

void ConnectlessManager::Kernel()
{
    Merge();
    OnRto();
    OS::SleepMS(1);
}

void ConnectlessManager::Merge()
{
    // info.slot>capacity  --> add object
    // info.rto==0  --> remove object
    // info.stamp==0  --> idle object
    // info.stamp==?&&info.rto==?  --> relocate object
    std::shared_ptr<ConnInfo> infoPtr;
    while (chan_rto_objs_.Dequeue(infoPtr)) {
        auto& info = *infoPtr;
        if (info.slot > capacity) {
            rto_objs_[capacity].push_back(infoPtr);
            info.itr = --rto_objs_[capacity].end();
            info.slot = capacity;
        } else if (info.rto == 0) {
            if (info.slot <= capacity) {
                rto_objs_[info.slot].erase(info.itr);
                info.slot = capacity + 2;
            }
        } else {
            auto slot = info.stamp == 0 ? capacity : std::max
                (info.stamp + info.rto, tick_count_) % capacity;
            if (slot != info.slot) {
                rto_objs_[info.slot].erase(info.itr);
                rto_objs_[slot].push_back(infoPtr);
                info.itr = --rto_objs_[slot].end();
                info.slot = slot;
            }
        }
    }
}

void ConnectlessManager::OnRto()
{
    size_t ranges[2][2] = {};
    uint32 goto_tick_count = u32(GET_REAL_APP_TIME);
    if (goto_tick_count - tick_count_ >= capacity) {
        ranges[0][0] = 0, ranges[0][1] = capacity;
    } else if (tick_count_ < goto_tick_count) {
        size_t start = tick_count_ % capacity;
        size_t stop = goto_tick_count % capacity;
        if (start < stop) {
            ranges[0][0] = start, ranges[0][1] = stop;
        } else {
            ranges[0][0] = start, ranges[0][1] = capacity;
            ranges[1][0] = 0, ranges[1][1] = stop;
        }
    }

    for (const auto& range : ranges) {
        for (auto i = range[0]; i < range[1]; ++i) {
            PostRotRequest(rto_objs_[i]);
        }
    }

    tick_count_ = goto_tick_count;
}

void ConnectlessManager::PostRotRequest(const std::list<std::weak_ptr<ConnInfo>> &objs)
{
    for (const auto& infoPtr : objs) {
        if (!infoPtr.expired()) {
            const auto& info = *infoPtr.lock();
            info.conn->PostPktRtoRequest(info.rto);
        } else {
            ELOG("ConnInfo is expired.");
        }
    }
}

std::string ConnectlessManager::NewUniqueKey()
{
    static std::atomic<uint32> counter{0};
    auto key = counter.fetch_add(1);
    return{(const char *)&key, sizeof(key)};
}
