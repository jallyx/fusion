#include "Session.h"
#include "SessionManager.h"
#include "ConnectionManager.h"
#include "System.h"
#include "Logger.h"

Session::Session()
: status_(Idle)
, manager_(nullptr)
, event_observer_(nullptr)
, is_overstocked_packet_(false)
, overflow_packet_count_(0)
, shutdown_time_(-1)
, last_recv_pck_time_(GET_APP_TIME)
, last_send_pck_time_(GET_APP_TIME)
{
}

Session::~Session()
{
    ClearRecvPacket();
}

void Session::Update()
{
    uint32 opcode = 0;
    INetPacket *pck = nullptr;
    TRY_BEGIN {

        while (IsActive() && recv_queue_.Dequeue(pck)) {
            opcode = pck->GetOpcode();
            switch (HandlePacket(pck)) {
            case SessionHandleSuccess:
                break;
            case SessionHandleCapture:
                pck = nullptr;
                break;
            case SessionHandleUnhandle:
                DLOG("SessionHandleUnhandle Opcode[%u].", opcode);
                break;
            case SessionHandleWarning:
                WLOG("Handle Opcode[%u] Warning!", opcode);
                break;
            case SessionHandleError:
                ELOG("Handle Opcode[%u] Error!", opcode);
                break;
            case SessionHandleKill:
            default:
                WLOG("Fatal error occurred when processing opcode[%u], "
                     "the session has been removed.", opcode);
                ShutdownSession();
                break;
            }
            SAFE_DELETE(pck);
        }

    } TRY_END
    CATCH_BEGIN(const IException &e) {
        WLOG("Handle session opcode[%u] exception occurred.", opcode);
        e.Print();
        KillSession();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("Handle session opcode[%u] unknown exception occurred.", opcode);
        KillSession();
    } CATCH_END

    SAFE_DELETE(pck);
}

void Session::ConnectServer(const std::string &address, const std::string &port)
{
    std::shared_ptr<Connection> connPtr = sConnectionManager.NewConnection(*this);
    connPtr->AsyncConnect(address, port);
    sSessionManager.AddSession(this);
}

void Session::SetConnection(std::shared_ptr<Connection> &&conn)
{
    connection_ = std::move(conn);
}

const std::shared_ptr<Connection> &Session::GetConnection() const
{
    return connection_;
}

void Session::DeleteObject()
{
    delete this;
}

void Session::Disconnect()
{
    if (connection_ && connection_->IsActive()) {
        connection_->PostCloseRequest();
    }
}

bool Session::IsIndependent() const
{
    return !connection_ || connection_.unique();
}

bool Session::HasSendDataAwaiting() const
{
    return connection_->HasSendDataAwaiting();
}

size_t Session::GetSendDataSize() const
{
    return connection_->GetSendDataSize();
}

int Session::GetConnectionLoadValue() const
{
    return 1;
}

const std::string &Session::GetHost() const
{
    return connection_->addr();
}

unsigned long Session::GetIPv4() const
{
    boost::asio::ip::address addr;
    addr.from_string(connection_->addr().c_str());
    return addr.is_v4() ? addr.to_v4().to_ulong() : 0;
}

unsigned short Session::GetPort() const
{
    return connection_->port();
}

void Session::ClearShutdownFlag()
{
    shutdown_time_.store(-1);
}

bool Session::GrabShutdownFlag()
{
    time_t expected = -1;
    return shutdown_time_.compare_exchange_strong(expected, GET_UNIX_TIME);
}

bool Session::IsShutdownExpired() const
{
    return shutdown_time_.load() + 30 < GET_UNIX_TIME;
}

void Session::SetEventObserver(IEventObserver *observer)
{
    event_observer_ = observer;
}

void Session::ClearPacketOverstockFlag()
{
    is_overstocked_packet_ = false;
}

void Session::KillSession()
{
    if (manager_ != nullptr) {
        manager_->KillSession(this);
    }
}

void Session::ShutdownSession()
{
    if (manager_ != nullptr) {
        manager_->ShutdownSession(this);
    }
}

void Session::OnShutdownSession()
{
    ClearRecvPacket();
    if (event_observer_ != nullptr) {
        event_observer_->OnShutdownSession(this);
    }
}

void Session::OnRecvPacket(INetPacket *pck)
{
    recv_queue_.Enqueue(pck);
    if (!is_overstocked_packet_ && event_observer_ != nullptr) {
        is_overstocked_packet_ = true;
        event_observer_->OnRecvPacket(this);
    }
}

void Session::PushRecvPacket(INetPacket *pck)
{
    if (IsActive()) {
        if (pck->GetOpcode() == OPCODE_LARGE_PACKET) {
            PushRecvFragmentPacket(pck);
        } else {
            OnRecvPacket(pck);
        }
        last_recv_pck_time_ = GET_APP_TIME;
    } else {
        delete pck;
    }
}

void Session::PushSendPacket(const INetPacket &pck)
{
    if (IsActive() && connection_) {
        if (pck.GetReadableSize() > INetPacket::MAX_BUFFER_SIZE) {
            PushSendOverflowPacket(pck);
        } else {
            connection_->GetSendBuffer().WritePacket(pck);
        }
        connection_->PostWriteRequest();
        last_send_pck_time_ = GET_APP_TIME;
    }
}

void Session::PushSendPacket(const char *data, size_t size)
{
    if (IsActive() && connection_) {
        connection_->GetSendBuffer().WritePacket(data, size);
        connection_->PostWriteRequest();
        last_send_pck_time_ = GET_APP_TIME;
    }
}

void Session::PushSendPacket(const INetPacket &pck, const INetPacket &data)
{
    if (IsActive() && connection_) {
        if (pck.GetReadableSize() + data.GetReadableSize() +
                INetPacket::Header::SIZE > INetPacket::MAX_BUFFER_SIZE) {
            PushSendOverflowPacket(pck, data);
        } else {
            connection_->GetSendBuffer().WritePacket(pck, data);
        }
        connection_->PostWriteRequest();
        last_send_pck_time_ = GET_APP_TIME;
    }
}

void Session::PushSendPacket(const INetPacket &pck, const char *data, size_t size)
{
    if (IsActive() && connection_) {
        if (pck.GetReadableSize() + size > INetPacket::MAX_BUFFER_SIZE) {
            PushSendOverflowPacket(pck, data, size);
        } else {
            connection_->GetSendBuffer().WritePacket(pck, data, size);
        }
        connection_->PostWriteRequest();
        last_send_pck_time_ = GET_APP_TIME;
    }
}

class Session::LargePacketHelper {
public:
    static INetPacket &UnpackPacket(INetPacket &pck) {
        const uint32 len = pck.Read<uint32>();
        pck.SetOpcode(pck.Read<uint16>());
        if (len != pck.GetReadableSize() + LARGE_PACKET_HEADER_SIZE) {
            THROW_EXCEPTION(NetStreamException());
        }
        return pck;
    }
    static void WritePacketHeader(INetPacket &pck, size_t size, uint32 opcode) {
        pck.Write<uint32>(size + LARGE_PACKET_HEADER_SIZE);
        pck.Write<uint16>(opcode);
    }
    static const size_t LARGE_PACKET_HEADER_SIZE = 4 + 2;
};

void Session::PushRecvFragmentPacket(INetPacket *pck)
{
    const size_t packet_total_size = pck->GetTotalSize();
    const uint32 number = pck->Read<uint32>();
    do {
        std::lock_guard<std::mutex> lock(fragment_mutex_);
        INetPacket *&packet = fragment_packets_[number];
        if (packet == nullptr) {
            packet = pck;
        } else {
            packet->Append(pck->GetReadableBuffer(), pck->GetReadableSize());
            delete pck;
        }
        if (packet_total_size < INetPacket::MAX_BUFFER_SIZE) {
            OnRecvPacket(&LargePacketHelper::UnpackPacket(*packet));
            fragment_packets_.erase(number);
        }
    } while (0);
}

void Session::PushSendOverflowPacket(const INetPacket &pck)
{
    ConstNetBuffer datas[] = {
        { pck.GetReadableBuffer(), pck.GetReadableSize() },
    };
    PushSendFragmentPacket(pck.GetOpcode(), datas, ARRAY_SIZE(datas));
}

void Session::PushSendOverflowPacket(const INetPacket &pck, const INetPacket &data)
{
    TNetPacket<INetPacket::Header::SIZE> wrapper;
    wrapper.WriteHeader(INetPacket::Header(data.GetOpcode(),
        (data.GetReadableSize() + INetPacket::Header::SIZE) & 0xffff));

    ConstNetBuffer datas[] = {
        { pck.GetReadableBuffer(), pck.GetReadableSize() },
        { wrapper.GetBuffer(), wrapper.GetTotalSize() },
        { data.GetReadableBuffer(), data.GetReadableSize() },
    };
    PushSendFragmentPacket(pck.GetOpcode(), datas, ARRAY_SIZE(datas));
}

void Session::PushSendOverflowPacket(const INetPacket &pck, const char *data, size_t size)
{
    ConstNetBuffer datas[] = {
        { pck.GetReadableBuffer(), pck.GetReadableSize() },
        { data, size },
    };
    PushSendFragmentPacket(pck.GetOpcode(), datas, ARRAY_SIZE(datas));
}

void Session::PushSendFragmentPacket(uint32 opcode, ConstNetBuffer datas[], size_t count)
{
    size_t data_total_size = 0;
    for (size_t i = 0; i < count; ++i) {
        data_total_size += datas[i].GetTotalSize();
    }

    NetPacket packet(OPCODE_LARGE_PACKET);
    packet << (uint32)overflow_packet_count_.fetch_add(1);
    const size_t packet_prefix_size = packet.GetTotalSize();
    LargePacketHelper::WritePacketHeader(packet, data_total_size, opcode);

    bool is_residual_data = true;
    for (size_t i = 0; i < count; ++i) {
        auto &data = datas[i];
        while (!data.IsReadableEmpty()) {
            const char *data_buffer = data.GetReadableBuffer();
            const size_t packet_space_size = INetPacket::MAX_BUFFER_SIZE - packet.GetTotalSize();
            const size_t data_avail_size = std::min(data.GetReadableSize(), packet_space_size);
            if (data_avail_size < packet_space_size && data_avail_size < data_total_size) {
                packet.Append(data_buffer, data_avail_size);
            } else {
                connection_->GetSendBuffer().WritePacket(packet, data_buffer, data_avail_size);
                is_residual_data = data_avail_size >= packet_space_size;
                packet.Shrink(packet_prefix_size);
            }
            data.AdjustReadPos(data_avail_size);
            data_total_size -= data_avail_size;
        }
    }

    if (is_residual_data) {
        connection_->GetSendBuffer().WritePacket(packet);
    }

    DBGASSERT(data_total_size == 0);
}

void Session::ClearRecvPacket()
{
    INetPacket *pck = nullptr;
    while (recv_queue_.Dequeue(pck)) {
        delete pck;
    }
    do {
        std::lock_guard<std::mutex> lock(fragment_mutex_);
        for (auto &pair : fragment_packets_) {
            delete pair.second;
        }
        fragment_packets_.clear();
    } while (0);
}
