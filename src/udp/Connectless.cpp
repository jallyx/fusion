#include "Connectless.h"
#include "ConnectlessManager.h"
#include "Sessionless.h"
#include "System.h"
#include <functional>

Connectless::Connectless(asio::io_service &io_service,
    ConnectlessManager &manager, Sessionless &sessionless)
: manager_(manager)
, sessionless_(sessionless)
, is_active_(false)
, strand_(io_service)
, resolver_(io_service)
, port_(0)
, is_connected_(false)
, final_recv_buffer_(MAX_NET_PACKET_SIZE+1)
, is_residual_send_data_(false)
, rtt_(1000)
, recv_sn_{0,0}
, send_sn_{{0},{0}}
, is_recving_{ATOMIC_FLAG_INIT}
, is_reading_{ATOMIC_FLAG_INIT}
, is_writing_{ATOMIC_FLAG_INIT}
, last_recv_data_time_(GET_SYS_TIME)
, last_send_data_time_(GET_SYS_TIME)
{
}

Connectless::~Connectless()
{
    Close();
    for (; !packets_.empty(); packets_.pop()) {
        delete packets_.top().pck;
    }
}

void Connectless::Init()
{
    sessionless_.SetConnectless(shared_from_this());
}

void Connectless::Close()
{
    if (is_active_) {
        is_active_ = is_connected_ = false;
        WLOG("Close connectless from [%s:%hu].", addr_.c_str(), port_);

        manager_.RemoveConnectless(key());
        if (sessionless_.IsActive()) {
            sessionless_.KillSession();
        }

        if (sock_ && dest_.address().is_unspecified()) {
            sock_->close();
        }
    }
}

void Connectless::SetSocket(const std::string &key,
        const std::shared_ptr<asio::ip::udp::socket> &socket,
        const asio::ip::udp::socket::endpoint_type &destination)
{
    is_active_ = is_connected_ = true;
    sock_ = socket;
    dest_ = destination;
    key_ = key;
    addr_ = destination.address().to_string();
    port_ = destination.port();
}

void Connectless::AsyncConnect(const std::string &key,
        const std::string &address, const std::string &port)
{
    is_active_ = true;
    sock_ = std::make_shared<asio::ip::udp::socket>(strand_.get_io_service());
    key_ = key;
    addr_ = address;
    port_ = atoi(port.c_str());

    asio::ip::udp::resolver::query query(address, port);
    resolver_.async_resolve(query, strand_.wrap(
        std::bind(&Connectless::OnResolveComplete, shared_from_this(),
            std::placeholders::_1, std::placeholders::_2)));
}

void Connectless::SendBufferData(const void *data, size_t size)
{
    MultiBuffers buffers;
    buffers.add(data, size);
    SendPkt(buffers);
}

void Connectless::SendPacketReliable(const INetPacket &pck)
{
    NetPacket packet(pck.GetOpcode());
    packet << send_sn_[0].fetch_add(1) + 1;
    send_buffer_.WritePacket(
        packet, pck.GetReadableBuffer(), pck.GetReadableSize());
    PostWriteRequest();
}

void Connectless::SendPacketUnreliable(const INetPacket &pck)
{
    NetBuffer buffer;
    buffer << u8(2) << send_sn_[0].load()
        << send_sn_[1].fetch_add(1) << (u16)pck.GetOpcode();
    MultiBuffers buffers;
    buffers.add(buffer.GetBuffer(), buffer.GetTotalSize());
    buffers.add(pck.GetReadableBuffer(), pck.GetReadableSize());
    SendPkt(buffers);
}

void Connectless::SendAck(uint8 age, uint64 seq, uint64 una)
{
    NetBuffer buffer;
    buffer << u8(3) << age << seq << una;
    MultiBuffers buffers;
    buffers.add(buffer.GetBuffer(), buffer.GetTotalSize());
    SendPkt(buffers);
}

void Connectless::OnPktAck(uint8 age, uint64 seq, uint64 una)
{
    SendQueue::OnSentPktRst rst;
    final_send_queue_.OnPktSent(age, seq, una, rst);
    if (rst.stamp != 0) {
        rtt_ = u32(GET_REAL_APP_TIME) - rst.stamp;
        StartLostWrite(rst.next);
    }
    if (!is_writing_.test_and_set()) {
        StartDataWrite();
    }
}

void Connectless::OnRecvPkt(uint8 type, ConstNetBuffer &buffer)
{
    switch (type) {
        case 1: {
            uint8 age;
            uint64 seq;
            buffer >> age >> seq;
            if (!recv_queue_.overflow(buffer.GetReadableSize(), seq)) {
                recv_queue_.OnPktRecv(
                    buffer.GetReadableBuffer(), buffer.GetReadableSize(), seq);
                SendAck(age, seq, recv_queue_.seq());
                PostReadRequest();
            }
            break;
        }
        case 2: {
            PacketInfo info;
            buffer >> info.sn[0] >> info.sn[1];
            if (info.sn[0] >= recv_sn_[0]) {
                const uint16 opcode = buffer.Read<uint16>();
                info.pck = INetPacket::New(opcode, buffer.GetReadableSize());
                (*info.pck).Append(
                    buffer.GetReadableBuffer(), buffer.GetReadableSize());
                strand_.dispatch(std::bind(
                    &Connectless::DispatchPacketUnreliable, shared_from_this(), info));
            }
            break;
        }
        case 3: {
            uint8 age;
            uint64 seq, una;
            buffer >> age >> seq >> una;
            strand_.dispatch(std::bind(
                &Connectless::OnPktAck, shared_from_this(), age, seq, una));
            break;
        }
    }
    last_recv_data_time_ = GET_SYS_TIME;
}

void Connectless::PostRecvRequest()
{
    if (!is_recving_.test_and_set()) {
        strand_.dispatch(std::bind(&Connectless::StartNextRecv, shared_from_this()));
    }
}

void Connectless::PostReadRequest()
{
    if (!is_reading_.test_and_set()) {
        strand_.dispatch(std::bind(&Connectless::StartDataRead, shared_from_this()));
    }
}

void Connectless::PostWriteRequest()
{
    if (IsConnected() && !is_writing_.test_and_set()) {
        strand_.dispatch(std::bind(&Connectless::StartDataWrite, shared_from_this()));
    }
}

void Connectless::PostCloseRequest()
{
    if (IsActive()) {
        strand_.dispatch(std::bind(&Connectless::Close, shared_from_this()));
    }
}

void Connectless::PostPktRtoRequest(uint32 rto)
{
    if (IsActive()) {
        strand_.dispatch(std::bind(&Connectless::StartRtoWrite, shared_from_this(), rto));
    }
}

void Connectless::StartNextRecv()
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        auto buffer = asio::buffer(async_recv_buffer, sizeof(async_recv_buffer));
        sock_->async_receive(buffer, strand_.wrap(
            std::bind(&Connectless::OnRecvComplete, shared_from_this(),
                      std::placeholders::_1, std::placeholders::_2)));

    } TRY_END
    CATCH_BEGIN(const asio::system_error &e) {
        WLOG("StartNextRecv[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("StartNextRecv[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("StartNextRecv[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connectless::StartDataRead()
{
    TRY_BEGIN {

        while (IsActive() && recv_queue_.HasRecvDataAvail()) {
            PreprocessRecvData();
            while (IsActive()) {
                PacketInfo info = ReadPacketReliableFromBuffer();
                if (info.pck != nullptr) {
                    DispatchPacket(info.pck);
                    recv_sn_[0] = std::max(recv_sn_[0], info.sn[0]);
                    for (; !packets_.empty(); packets_.pop()) {
                        auto &packet = packets_.top();
                        if (packet.sn[0] <= recv_sn_[0]) {
                            DispatchPacketUnreliable(packet);
                        } else {
                            break;
                        }
                    }
                } else {
                    break;
                }
            }
        }

        is_reading_.clear();
        if (IsActive()) {
            if (recv_queue_.HasRecvDataAvail() && !is_reading_.test_and_set()) {
                StartDataRead();
            }
        }

    } TRY_END
    CATCH_BEGIN(const asio::system_error &e) {
        WLOG("StartDataRead[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("StartDataRead[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("StartDataRead[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connectless::StartDataWrite()
{
    TRY_BEGIN {

        while (IsActive() && send_buffer_.HasSendDataAvail() && !final_send_queue_.IsFull()) {
            PreprocessSendData();
            while (IsActive()) {
                auto stamp = uint32(GET_REAL_APP_TIME);
                auto pkt = final_send_queue_.SendRtoPkt(stamp, stamp);
                if (!pkt.empty()) {
                    SendPkt(pkt);
                } else {
                    break;
                }
            }
        }

        is_writing_.clear();
        if (IsActive()) {
            if (send_buffer_.HasSendDataAvail() && !final_send_queue_.IsFull() && !is_writing_.test_and_set()) {
                StartDataWrite();
            } else {
                manager_.RefreshConnectless(key(), final_send_queue_.GetFirstPktStamp(), rto());
            }
        }

    } TRY_END
    CATCH_BEGIN(const asio::system_error &e) {
        WLOG("StartDataWrite[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("StartDataWrite[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("StartDataWrite[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connectless::StartRtoWrite(uint32 rto)
{
    TRY_BEGIN {

        const size_t num = final_send_queue_.GetPktNum();
        for (size_t i = 0; i < num && IsActive(); ++i) {
            auto stamp = uint32(GET_REAL_APP_TIME);
            auto pkt = final_send_queue_.SendRtoPkt(stamp, rto);
            if (!pkt.empty()) {
                for (int i = 0; i < 2; ++i) {
                    SendPkt(pkt);
                }
            } else {
                break;
            }
        }

        if (IsActive()) {
            manager_.RefreshConnectless(key(), final_send_queue_.GetFirstPktStamp(), this->rto());
        }

    } TRY_END
    CATCH_BEGIN(const asio::system_error &e) {
        WLOG("StartRtoWrite[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("StartRtoWrite[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("StartRtoWrite[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connectless::StartLostWrite(const SendQueue::PktPtr &stop)
{
    TRY_BEGIN {

        const size_t num = final_send_queue_.GetPktNum();
        for (size_t i = 0; i < num && IsActive(); ++i) {
            auto stamp = uint32(GET_REAL_APP_TIME);
            auto pkt = final_send_queue_.SendLostPkt(stamp, stop);
            if (!pkt.empty()) {
                for (int i = 0; i < 2; ++i) {
                    SendPkt(pkt);
                }
            } else {
                break;
            }
        }

        if (IsActive()) {
            manager_.RefreshConnectless(key(), final_send_queue_.GetFirstPktStamp(), this->rto());
        }

    } TRY_END
    CATCH_BEGIN(const asio::system_error &e) {
        WLOG("StartLostWrite[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("StartLostWrite[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("StartLostWrite[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connectless::OnResolveComplete(const asio::error_code &ec, asio::ip::udp::resolver::iterator itr)
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        if (ec) {
            WLOG("Resolve connectless[%s:%hu], %s.", addr_.c_str(), port_, ec.message().c_str());
            Close();
            return;
        }

        last_recv_data_time_ = GET_SYS_TIME;
        last_send_data_time_ = GET_SYS_TIME;
        sock_->async_connect(*itr, strand_.wrap(
            std::bind(&Connectless::OnConnectComplete, shared_from_this(),
                      std::placeholders::_1)));

    } TRY_END
    CATCH_BEGIN(const asio::system_error &e) {
        WLOG("OnResolveComplete[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("OnResolveComplete[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("OnResolveComplete[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connectless::OnConnectComplete(const asio::error_code &ec)
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        if (ec) {
            WLOG("Connect connectless[%s:%hu], %s.", addr_.c_str(), port_, ec.message().c_str());
            Close();
            return;
        }

        last_recv_data_time_ = GET_SYS_TIME;
        last_send_data_time_ = GET_SYS_TIME;
        is_connected_ = true;
        sock_->non_blocking(true);
        PostRecvRequest();
        PostWriteRequest();
        sessionless_.OnReadyConnectless();

    } TRY_END
    CATCH_BEGIN(const asio::system_error &e) {
        WLOG("OnConnectComplete[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("OnConnectComplete[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("OnConnectComplete[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connectless::OnRecvComplete(const asio::error_code &ec, std::size_t bytes)
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        if (ec) {
            WLOG("Recv connectless[%s:%hu], %s.", addr_.c_str(), port_, ec.message().c_str());
            Close();
            return;
        }

        ConstNetBuffer buffer(async_recv_buffer, bytes);
        OnRecvPkt(buffer.Read<u8>(), buffer);
        StartNextRecv();

    } TRY_END
    CATCH_BEGIN(const asio::system_error &e) {
        WLOG("OnRecvComplete[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("OnRecvComplete[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("OnRecvComplete[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connectless::PreprocessRecvData()
{
    while (IsActive() && recv_queue_.HasRecvDataAvail() && !final_recv_buffer_.IsFull()) {
        size_t inlen = 0;
        const char *in = recv_queue_.GetRecvDataBuffer(inlen);
        size_t outlen = final_recv_buffer_.GetContiguiousWritableSpace();
        char *out = final_recv_buffer_.GetContiguiousWritableBuffer();
        if (inflate_stream_.Inflate(in, inlen, out, outlen)) {
            if (inlen != 0) {
                recv_queue_.RemoveRecvData(inlen);
            }
            if (outlen != 0) {
                final_recv_buffer_.IncrementContiguiousWritten(outlen);
            }
        } else {
            THROW_EXCEPTION(RecvDataException());
        }
    }
}

void Connectless::PreprocessSendData()
{
    SendQueue::DataWriter w;
    final_send_queue_.Prepare(w);

    while (IsActive() && send_buffer_.HasSendDataAvail() && !final_send_queue_.IsFull()) {
        size_t inlen = 0;
        const char *in = send_buffer_.GetSendDataBuffer(inlen);
        size_t availen = 0;
        char *out = final_send_queue_.GetWritableSpace(availen);
        auto outlen = availen = std::min(UDP_PKT_MAX - w.n, availen);
        if (deflate_stream_.Deflate(in, inlen, out, outlen)) {
            if (inlen != 0) {
                send_buffer_.RemoveSendData(inlen);
            }
            if (outlen != 0) {
                final_send_queue_.IncrementWritten(w, outlen);
            }
            if (w.n >= UDP_PKT_MAX) {
                final_send_queue_.Flush(w);
                final_send_queue_.Prepare(w);
            }
            is_residual_send_data_ = true;
        } else {
            THROW_EXCEPTION(SendDataException());
        }
    }

    while (is_residual_send_data_ && IsActive() && !final_send_queue_.IsFull()) {
        size_t availen = 0;
        char *out = final_send_queue_.GetWritableSpace(availen);
        auto outlen = availen = std::min(UDP_PKT_MAX - w.n, availen);
        if (deflate_stream_.Flush(out, outlen)) {
            if (outlen != 0) {
                final_send_queue_.IncrementWritten(w, outlen);
            }
            if (w.n >= UDP_PKT_MAX) {
                final_send_queue_.Flush(w);
                final_send_queue_.Prepare(w);
            }
            if (outlen < availen) {
                is_residual_send_data_ = false;
            }
        } else {
            THROW_EXCEPTION(SendDataException());
        }
    }

    if (w.n != 0) {
        final_send_queue_.Flush(w);
    } else {
        final_send_queue_.Revert(w);
    }
}

Connectless::PacketInfo Connectless::ReadPacketReliableFromBuffer()
{
    PacketInfo info{ {}, nullptr };
    if (final_recv_buffer_.GetReadableSpace() < INetPacket::Header::SIZE) {
        return info;
    }

    INetPacket::Header header;
    TNetPacket<INetPacket::Header::SIZE> wrapper;
    wrapper.Erlarge(INetPacket::Header::SIZE);
    final_recv_buffer_.Peek((char*)wrapper.GetBuffer(), wrapper.GetTotalSize());
    wrapper.ReadHeader(header);
    if (final_recv_buffer_.GetReadableSpace() < header.len) {
        return info;
    }

    info.pck = INetPacket::New(header.cmd, header.len - INetPacket::Header::SIZE);
    info.pck->Erlarge(header.len - INetPacket::Header::SIZE);
    final_recv_buffer_.Remove(INetPacket::Header::SIZE);
    final_recv_buffer_.Read((char*)info.pck->GetBuffer(), info.pck->GetTotalSize());

    (*info.pck) >> info.sn[0];
    return info;
}

void Connectless::DispatchPacketUnreliable(const PacketInfo &info)
{
    if (info.sn[0] == recv_sn_[0] && info.sn[1] > recv_sn_[1]) {
        DispatchPacket(info.pck), recv_sn_[1] = info.sn[1];
    } else if (info.sn[0] > recv_sn_[0]) {
        packets_.push(info);
    } else {
        delete info.pck;
    }
}

void Connectless::DispatchPacket(INetPacket *pck)
{
    sessionless_.PushRecvMsg(pck);
}

void Connectless::SendPkt(const SendQueue::PktValue &pkt)
{
    NetBuffer buffer;
    buffer << u8(1) << pkt.age() << pkt.seq();
    Connectless::MultiBuffers buffers;
    buffers.add(buffer.GetBuffer(), buffer.GetTotalSize());
    buffers.add(pkt);
    SendPkt(buffers);
}

void Connectless::SendPkt(const MultiBuffers &buffers)
{
    if (sock_) {
        asio::error_code ec;
        if (dest_.address().is_unspecified()) {
            sock_->send(buffers, 0, ec);
        } else {
            sock_->send_to(buffers, dest_, 0, ec);
        }
    }
    last_send_data_time_ = GET_SYS_TIME;
}

bool Connectless::HasSendDataAwaiting() const
{
    if (final_send_queue_.GetDataSize() != 0)
        return true;
    if (send_buffer_.HasSendDataAvail())
        return true;
    return false;
}

size_t Connectless::GetSendDataSize() const
{
    return final_send_queue_.GetDataSize() +
        send_buffer_.GetDataSize();
}
