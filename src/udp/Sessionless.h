#pragma once

#include "network/Session.h"

class Connectless;

class Sessionless : public Session
{
public:
    Sessionless(bool isRapidMode = false);
    virtual ~Sessionless();

    void SetConnectless(std::shared_ptr<Connectless> &&conn);
    const std::shared_ptr<Connectless> &GetConnectless() const;

    virtual void PushRecvMsg(INetPacket *pck);

    virtual void PushSendReliable(const INetPacket &pck);
    virtual void PushSendUnreliable(const INetPacket &pck);
    virtual void PushSendBufferData(const void *data, size_t size);

    virtual void Disconnect();
    virtual bool IsIndependent() const;
    virtual bool HasSendDataAwaiting() const;
    virtual size_t GetSendDataSize() const;

    virtual void OnReadyConnectless() {}

private:
    std::shared_ptr<Connectless> connectless_;
    uint64 last_recv_pck_time_, last_send_pck_time_;
};
