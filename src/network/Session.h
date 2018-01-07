#pragma once

#include <memory>
#include <unordered_map>
#include "NetPacket.h"
#include "ThreadSafeDoubleQueue.h"

class SessionManager;
class Connection;

enum SessionHandleStatus {
    SessionHandleSuccess,
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

    class ISendBuffer {
    public:
        virtual ~ISendBuffer() = default;
        virtual void WritePacket(const INetPacket &pck) = 0;
        virtual void WritePacket(const char *data, size_t size) = 0;
        virtual void WritePacket(const INetPacket &pck, const INetPacket &data) = 0;
        virtual void WritePacket(const INetPacket &pck, const char *data, size_t size) = 0;
    };

    Session(bool isDeflatePacket = false, bool isInflatePacket = false, bool isRapidMode = false);
    virtual ~Session();

    void SetConnection(std::shared_ptr<Connection> &&connPtr);
    const std::shared_ptr<Connection> &GetConection() const;
    bool IsMonopolizeConection() const;

    void ResetShutdownFlag();
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
    virtual void OnShutdownSession() {}

    virtual void DeleteObject();
    virtual void CheckTimeout() {}

    void SetManager(SessionManager *manager) { manager_ = manager;}
    SessionManager *GetManager() const { return manager_; }

    void SetStatus(Status status) { status_ = status; }
    void Disable() { status_ = Disabled; }
    bool IsActive() const { return status_ != Disabled; }

    bool is_deflate_packet() const { return is_deflate_packet_; }
    bool is_inflate_packet() const { return is_inflate_packet_; }
    bool is_rapid_mode() const { return is_rapid_mode_; }

protected:
    class LargePacketHelper;
    void PushRecvFragmentPacket(INetPacket *pck);
    void PushSendOverflowPacket(const INetPacket &pck);

    void ClearRecvPacket();

private:
    const bool is_deflate_packet_;
    const bool is_inflate_packet_;
    const bool is_rapid_mode_;

    Status status_;
    SessionManager *manager_;

    std::shared_ptr<Connection> connection_;
    ISendBuffer *send_buffer_;
    ThreadSafeDoubleQueue<INetPacket*, 128> recv_queue_;

    std::mutex fragment_mutex_;
    std::unordered_map<uint32, INetPacket*> fragment_packets_;
    std::atomic<uint32> overflow_packet_count_;

    std::atomic<time_t> shutdown_time_;
    uint64 last_recv_pck_time_, last_send_pck_time_;
};
