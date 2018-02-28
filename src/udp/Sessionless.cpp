#include "Sessionless.h"
#include "Connectless.h"
#include "System.h"

Sessionless::Sessionless(bool isRapidMode)
: Session(true, true, isRapidMode)
, last_recv_pck_time_(GET_SYS_TIME)
, last_send_pck_time_(GET_SYS_TIME)
{
}

Sessionless::~Sessionless()
{
}

void Sessionless::SetConnectless(std::shared_ptr<Connectless> &&conn)
{
    connectless_ = std::move(conn);
}

const std::shared_ptr<Connectless> &Sessionless::GetConnectless() const
{
    return connectless_;
}

void Sessionless::PushRecvMsg(INetPacket *pck)
{
    if (IsActive()) {
        OnRecvPacket(pck);
        last_recv_pck_time_ = GET_SYS_TIME;
    } else {
        delete pck;
    }
}

void Sessionless::PushSendReliable(const INetPacket &pck)
{
    if (IsActive() && connectless_) {
        connectless_->SendPacketReliable(pck);
        last_send_pck_time_ = GET_SYS_TIME;
    }
}

void Sessionless::PushSendUnreliable(const INetPacket &pck)
{
    if (IsActive() && connectless_) {
        connectless_->SendPacketUnreliable(pck);
        last_send_pck_time_ = GET_SYS_TIME;
    }
}

void Sessionless::PushSendBufferData(const void *data, size_t size)
{
    if (IsActive() && connectless_) {
        connectless_->SendBufferData(data, size);
        last_send_pck_time_ = GET_SYS_TIME;
    }
}

void Sessionless::Disconnect()
{
    Session::Disconnect();
    if (connectless_ && connectless_->IsActive()) {
        connectless_->PostCloseRequest();
    }
}

bool Sessionless::IsIndependent() const
{
    return Session::IsIndependent() &&
        (connectless_ ? connectless_.unique() : true);
}

bool Sessionless::HasSendDataAwaiting() const
{
    return Session::HasSendDataAwaiting() ||
        (connectless_ ? connectless_->HasSendDataAwaiting() : false);
}

size_t Sessionless::GetSendDataSize() const
{
    return Session::GetSendDataSize() +
        (connectless_ ? connectless_->GetSendDataSize() : 0);
}
