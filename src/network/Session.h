#pragma once

#include <memory>
#include <unordered_map>
#include "NetPacket.h"
#include "ThreadSafeDoubleQueue.h"

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
        Connecting,
        Running,
        Disabled,
    };

    class ISendBuffer {
    public:
        virtual void WritePacket(const INetPacket &pck) = 0;
        virtual void WritePacket(const char *data, size_t size) = 0;
        virtual void WritePacket(const INetPacket &pck, const INetPacket &data) = 0;
        virtual void WritePacket(const INetPacket &pck, const char *data, size_t size) = 0;
        virtual size_t GetDataSize() const = 0;
    };

    class IEventObserver {
    public:
        virtual void OnRecvPacket(Session *session) = 0;
        virtual void OnShutdownSession(Session *session) = 0;
    };

    Session(bool isDeflatePacket = false, bool isInflatePacket = false, bool isRapidMode = false);
    virtual ~Session();

    void SetConnection(std::shared_ptr<Connection> &&connPtr);
    const std::shared_ptr<Connection> &GetConnection() const;
    bool IsMonopolizeConnection() const;

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

    const std::string &GetHost() const;
    unsigned long GetIPv4() const;
    unsigned short GetPort() const;

    size_t GetSendDataSize() const { return send_buffer_->GetDataSize(); }

    void SetManager(SessionManager *manager) { manager_ = manager;}
    SessionManager *GetManager() const { return manager_; }

    void SetStatus(Status status) { status_ = status; }
    bool IsStatus(Status status) const { return status_ == status; }
    void Disable() { status_ = Disabled; }
    bool IsActive() const { return status_ != Disabled; }

    bool is_deflate_packet() const { return is_deflate_packet_; }
    bool is_inflate_packet() const { return is_inflate_packet_; }
    bool is_rapid_mode() const { return is_rapid_mode_; }

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

    const bool is_deflate_packet_;
    const bool is_inflate_packet_;
    const bool is_rapid_mode_;

    Status status_;
    SessionManager *manager_;

    std::shared_ptr<Connection> connection_;
    ISendBuffer *send_buffer_;
    ThreadSafeDoubleQueue<INetPacket*, 128> recv_queue_;

    IEventObserver *event_observer_;
    bool is_overstocked_packet_;

    std::mutex fragment_mutex_;
    std::unordered_map<uint32, INetPacket*> fragment_packets_;
    std::atomic<uint32> overflow_packet_count_;

    std::atomic<time_t> shutdown_time_;
    uint64 last_recv_pck_time_, last_send_pck_time_;
};
