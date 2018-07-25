#pragma once

#include <memory>
#include <unordered_map>
#include "NetBuffer.h"
#include "NetPacket.h"
#include "MultiBufferQueue.h"

class SessionManager;
class Connection;

enum SessionHandleStatus {
    SessionHandleSuccess,
    SessionHandleCapture,
    SessionHandleUnhandle,
    SessionHandleWarning,
    SessionHandleError,
    SessionHandleKill,
};

class Session
{
public:
    enum Status {
        Idle,
        Running,
        Disabled,
    };

    class IEventObserver {
    public:
        virtual void OnRecvPacket(Session *session) = 0;
        virtual void OnShutdownSession(Session *session) = 0;
    };

    Session();
    virtual ~Session();

    void ConnectServer(const std::string &address, const std::string &port);

    void SetConnection(std::shared_ptr<Connection> &&conn);
    const std::shared_ptr<Connection> &GetConnection() const;

    void SetEventObserver(IEventObserver *observer);
    void ClearPacketOverstockFlag();

    bool GrabShutdownFlag();
    bool IsShutdownExpired() const;

    void Update();
    virtual int HandlePacket(INetPacket *pck) = 0;

    virtual void PushRecvPacket(INetPacket *pck);

    virtual void PushSendPacket(const INetPacket &pck);
    virtual void PushSendPacket(const char *data, size_t size);
    virtual void PushSendPacket(const INetPacket &pck, const INetPacket &data);
    virtual void PushSendPacket(const INetPacket &pck, const char *data, size_t size);

    virtual void KillSession();
    virtual void ShutdownSession();
    virtual void OnShutdownSession();

    virtual void DeleteObject();
    virtual void CheckTimeout() {}
    virtual void OnConnected() {}
    virtual void OnManaged() {}

    virtual void Disconnect();
    virtual bool IsIndependent() const;
    virtual bool HasSendDataAwaiting() const;
    virtual size_t GetSendDataSize() const;

    virtual int GetConnectionLoadValue() const;

    const std::string &GetHost() const;
    unsigned long GetIPv4() const;
    unsigned short GetPort() const;

    void SetManager(SessionManager *manager) { manager_ = manager;}
    SessionManager *GetManager() const { return manager_; }

    void SetStatus(Status status) { status_ = status; }
    bool IsStatus(Status status) const { return status_ == status; }
    void Disable() { status_ = Disabled; }
    bool IsActive() const { return status_ != Disabled; }

    uint64 last_recv_pck_time() const { return last_recv_pck_time_; }
    uint64 last_send_pck_time() const { return last_send_pck_time_; }

protected:
    virtual void OnRecvPacket(INetPacket *pck);

    void ClearShutdownFlag();
    void ClearRecvPacket();

private:
    class LargePacketHelper;
    void PushRecvFragmentPacket(INetPacket *pck);
    void PushSendOverflowPacket(const INetPacket &pck);
    void PushSendOverflowPacket(const INetPacket &pck, const INetPacket &data);
    void PushSendOverflowPacket(const INetPacket &pck, const char *data, size_t size);
    void PushSendFragmentPacket(uint32 opcode, ConstNetBuffer datas[], size_t count);

    Status status_;
    SessionManager *manager_;

    std::shared_ptr<Connection> connection_;
    MultiBufferQueue<INetPacket*> recv_queue_;

    IEventObserver *event_observer_;
    bool is_overstocked_packet_;

    std::mutex fragment_mutex_;
    std::unordered_map<uint32, INetPacket*> fragment_packets_;
    std::atomic<uint32> overflow_packet_count_;

    std::atomic<time_t> shutdown_time_;
    uint64 last_recv_pck_time_, last_send_pck_time_;
};
