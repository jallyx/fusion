#pragma once

#include "network/Session.h"
#include "async/AsyncTaskOwner.h"

#define DEF_RPC_TIMEOUT (60)

enum RPCError {
    RPCErrorNone,
    RPCErrorTimeout,
    RPCErrorInterrupt,
};

class RPCSession : public Session
{
public:
    RPCSession();
    virtual ~RPCSession();

    virtual void OnTick();

    void Request(const INetPacket &pck,
        const std::function<void(INetStream&, int32, bool)> &cb = nullptr,
        AsyncTaskOwner *owner = nullptr, time_t timeout = DEF_RPC_TIMEOUT);

    void Reply(const INetPacket &pck,
        uint64 sn, int32 err = RPCErrorNone, bool eof = true);

protected:
    void SendAllRequests();
    void InterruptAllRequests();

    void OnRPCReply(INetPacket *pck);

    struct RequestMetaInfo {
        uint64 sn;
    };
    struct ReplyMetaInfo {
        uint64 sn;
        int32 err;
        bool eof;
    };
    int DoReply(INetPacket *pck, const ReplyMetaInfo &info);
    static RequestMetaInfo ReadRequestMetaInfo(INetPacket &pck);
    static ReplyMetaInfo ReadReplyMetaInfo(INetPacket &pck);

    bool is_ready_;

private:
    class RPCAsyncTask;
    struct TickInfo {
        uint64 sn;
        time_t expiry;
    };
    struct RequestInfo {
        INetPacket *pck;
        std::string args;
        RPCAsyncTask *task;
        std::weak_ptr<AsyncTaskOwner> owner;
        time_t timeout;
        size_t slot;
        std::list<TickInfo>::iterator itr;
    };

    void TickObjs();

    void RemoveTickObj(RequestInfo &info);
    void RelocateTickObj(RequestInfo &info, time_t timeout,
        std::list<TickInfo> &other, std::list<TickInfo>::iterator itr);

    std::vector<std::list<TickInfo>> tick_objs_;
    time_t tick_time_;
    std::unordered_map<uint64, RequestInfo> requests_;
    std::atomic<uint64> request_sn_;
    std::mutex mutex_;

    static AsyncTaskOwner owner_;
    static ConstNetPacket packet_;
};
