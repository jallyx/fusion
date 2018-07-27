#include "RPCSession.h"
#include "System.h"
#include "async/AsyncTask.h"

#define DEF_RPC_TICK_SIZE (256)

class RPCSession::RPCAsyncTask : public AsyncTask {
public:
    RPCAsyncTask(const std::function<void(INetStream&, int32, bool)> &cb)
        : cb_(cb), pck_(nullptr), err_(0), eof_(false)
    {}
    virtual ~RPCAsyncTask() {
        SAFE_DELETE(pck_);
    }
    virtual void Finish(AsyncTaskOwner *owner) {
        cb_(pck_ != nullptr ? *pck_ : RPCSession::packet_, err_, eof_);
    }
    virtual void ExecuteInAsync() {
    }
    void SetResult(INetPacket *pck, int32 err, bool eof) {
        pck_ = pck, err_ = err, eof_ = eof;
    }
private:
    const std::function<void(INetStream&, int32, bool)> cb_;
    INetPacket *pck_;
    int32 err_;
    bool eof_;
};

AsyncTaskOwner RPCSession::owner_;
ConstNetPacket RPCSession::packet_("", 0);

RPCSession::RPCSession()
: is_ready_(false)
, tick_objs_(DEF_RPC_TICK_SIZE)
, tick_time_(GET_UNIX_TIME)
{
}

RPCSession::~RPCSession()
{
    for (auto &pair : requests_) {
        SAFE_DELETE(pair.second.pck);
        SAFE_DELETE(pair.second.task);
    }
}

void RPCSession::OnTick()
{
    TickObjs();
    Session::OnTick();
}

void RPCSession::Request(const INetPacket &pck,
        const std::function<void(INetStream&, int32, bool)> &cb,
        AsyncTaskOwner *owner, time_t timeout)
{
    uint64 requestSN = request_sn_.fetch_add(1);
    NetBuffer args(requestSN);

    RequestInfo requestInfo{ nullptr, {}, nullptr, {} };
    requestInfo.pck = pck.Clone();
    requestInfo.args = args.CastBufferString();
    if (cb) {
        requestInfo.task = new RPCAsyncTask(cb);
        if (owner != nullptr) {
            owner->AddSubject(requestInfo.task);
            requestInfo.owner = owner->linked_from_this();
        } else {
            requestInfo.owner = owner_.linked_from_this();
        }
    }

    do {
        std::list<TickInfo> other{ { requestSN } };
        std::unique_lock<std::mutex> lock(mutex_);
        auto rst = requests_.emplace(requestSN, std::move(requestInfo));
        RelocateTickObj(rst.first->second, timeout, other, other.begin());
    } while (0);

    if (is_ready_) {
        PushSendPacket(pck, args.GetBuffer(), args.GetTotalSize());
    }
}

void RPCSession::Reply(const INetPacket &pck, uint64 sn, int32 err, bool eof)
{
    NetBuffer args(sn, err, eof);
    PushSendPacket(pck, args.GetBuffer(), args.GetTotalSize());
}

void RPCSession::SendAllRequests()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &pair : requests_) {
        auto &queryInfo = pair.second;
        PushSendPacket(*queryInfo.pck,
            queryInfo.args.data(), queryInfo.args.size());
    }
}

void RPCSession::InterruptAllRequests()
{
    std::list<TickInfo> others;
    do {
        std::unique_lock<std::mutex> lock(mutex_);
        for (auto &objs : tick_objs_) {
            others.splice(others.end(), objs);
        }
    } while (0);
    for (auto &info : others) {
        DoReply(nullptr, { info.sn, RPCErrorInterrupt, true });
    }
}

void RPCSession::OnRPCReply(INetPacket *pck)
{
    ReplyMetaInfo info = ReadReplyMetaInfo(*pck);
    if (DoReply(pck, info) != SessionHandleCapture) {
        delete pck;
    }
}

int RPCSession::DoReply(INetPacket *pck, const ReplyMetaInfo &info)
{
    RequestInfo requestInfo{ nullptr };
    do {
        std::unique_lock<std::mutex> lock(mutex_);
        auto itr = requests_.find(info.sn);
        if (itr != requests_.end()) {
            if (info.eof) {
                requestInfo = std::move(itr->second);
                requests_.erase(itr);
            } else {
                requestInfo = itr->second;
                RelocateTickObj(itr->second, requestInfo.timeout,
                    tick_objs_[requestInfo.slot], requestInfo.itr);
                if (requestInfo.task != nullptr) {
                    requestInfo.task = new RPCAsyncTask(*requestInfo.task);
                }
            }
        }
    } while (0);
    if (requestInfo.pck == nullptr) {
        return SessionHandleSuccess;
    }

    if (info.eof) {
        RemoveTickObj(requestInfo);
        delete requestInfo.pck;
    }
    if (requestInfo.task == nullptr) {
        return SessionHandleSuccess;
    }

    requestInfo.task->SetResult(pck, info.err, info.eof);
    if (!requestInfo.owner.expired()) {
        if (requestInfo.owner.lock().get() != &owner_) {
            requestInfo.owner.lock()->AddTask(requestInfo.task);
            requestInfo.task = nullptr;
        } else {
            TRY_BEGIN {
                requestInfo.task->Finish(nullptr);
            } TRY_END
            CATCH_BEGIN(const IException &e) {
                e.Print();
            } CATCH_END
            CATCH_BEGIN(...) {
            } CATCH_END
        }
    }
    if (requestInfo.task != nullptr) {
        delete requestInfo.task;
    }

    return SessionHandleCapture;
}

RPCSession::ReplyMetaInfo RPCSession::ReadReplyMetaInfo(INetPacket &pck)
{
    ReplyMetaInfo info;
    size_t infoLen = sizeof(info.sn) + sizeof(info.err) + sizeof(info.eof);
    if (pck.GetReadableSize() < infoLen) {
        THROW_EXCEPTION(NetStreamException());
    }

    size_t anchor = pck.GetReadPos();
    pck.AdjustReadPos(pck.GetReadableSize() - infoLen);
    pck >> info.sn >> info.err >> info.eof;
    pck.Shrink(pck.GetTotalSize() - infoLen);
    pck.AdjustReadPos(anchor - pck.GetReadPos());

    return info;
}

RPCSession::RequestMetaInfo RPCSession::ReadRequestMetaInfo(INetPacket &pck)
{
    RequestMetaInfo info;
    size_t infoLen = sizeof(info.sn);
    if (pck.GetReadableSize() < infoLen) {
        THROW_EXCEPTION(NetStreamException());
    }

    size_t anchor = pck.GetReadPos();
    pck.AdjustReadPos(pck.GetReadableSize() - infoLen);
    pck >> info.sn;
    pck.Shrink(pck.GetTotalSize() - infoLen);
    pck.AdjustReadPos(anchor - pck.GetReadPos());

    return info;
}

void RPCSession::TickObjs()
{
    time_t curTime = GET_UNIX_TIME;
    for (; tick_time_ < curTime; ++tick_time_) {
        std::list<TickInfo> others;
        auto &objs = tick_objs_[tick_time_ % DEF_RPC_TICK_SIZE];
        do {
            std::unique_lock<std::mutex> lock(mutex_);
            for (auto itr = objs.begin(); itr != objs.end();) {
                if (itr->expiry <= curTime) {
                    others.splice(others.end(), objs, itr++);
                } else {
                    ++itr;
                }
            }
        } while (0);
        for (auto &info : others) {
            DoReply(nullptr, { info.sn, RPCErrorTimeout, true });
        }
    }
}

void RPCSession::RemoveTickObj(RequestInfo &info)
{
    std::unique_lock<std::mutex> lock(mutex_);
    tick_objs_[info.slot].erase(info.itr);
}

void RPCSession::RelocateTickObj(RequestInfo &info, time_t timeout,
        std::list<TickInfo> &other, std::list<TickInfo>::iterator itr)
{
    time_t curTime = GET_UNIX_TIME;
    size_t slot = (curTime + info.timeout + 1) % DEF_RPC_TICK_SIZE;
    auto &objs = tick_objs_[slot];
    objs.splice(objs.end(), other, itr);
    info.timeout = timeout;
    info.slot = slot;
    info.itr = itr;
    itr->expiry = curTime + timeout;
}
