#include "Session.h"
#include "SessionManager.h"
#include "Connection.h"
#include "System.h"
#include "Logger.h"

Session::Session(bool isDeflatePacket, bool isInflatePacket, bool isRapidMode)
: is_deflate_packet_(isDeflatePacket)
, is_inflate_packet_(isInflatePacket)
, is_rapid_mode_(isRapidMode)
, status_(Idle)
, manager_(nullptr)
, send_buffer_(nullptr)
, overflow_packet_count_(0)
, shutdown_time_(-1)
, last_recv_pck_time_(GET_SYS_TIME)
, last_send_pck_time_(GET_SYS_TIME)
{
}

Session::~Session()
{
    ClearRecvPacket();
}

void Session::Update()
{
    INetPacket *pck = nullptr;
    TRY_BEGIN {

        bool isFirstSwap = true;
        while ((isFirstSwap || is_rapid_mode_) && IsActive() && recv_queue_.Swap()) {
            while (IsActive() && recv_queue_.Dequeue(pck)) {
                switch (HandlePacket(pck)) {
                case SessionHandleSuccess:
                    break;
                case SessionHandleUnhandle:
                    DLOG("SessionHandleUnhandle opcode[%u].", pck->GetOpcode());
                    break;
                case SessionHandleWarning:
                    WLOG("Handle Opcode[%u] Warning!", pck->GetOpcode());
                    break;
                case SessionHandleError:
                    ELOG("Handle Opcode[%u] Error!", pck->GetOpcode());
                    break;
                case SessionHandleKill:
                default:
                    WLOG("Fatal error occurred when processing opcode[%u], "
                         "the session has been removed.",
                         pck->GetOpcode());
                    ShutdownSession();
                    break;
                }
                SAFE_DELETE(pck);
            }
            isFirstSwap = false;
        }

    } TRY_END
    CATCH_BEGIN(const IException &e) {
        WLOG("Handle session opcode[%u] exception occurred.", pck->GetOpcode());
        e.Print();
        KillSession();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("Handle session opcode[%u] unknown exception occurred.", pck->GetOpcode());
        KillSession();
    } CATCH_END

    SAFE_DELETE(pck);
}

void Session::SetConnection(std::shared_ptr<Connection> &&connPtr)
{
    connection_ = std::move(connPtr);
    send_buffer_ = connection_->GetSendBuffer<ISendBuffer>();
}

const std::shared_ptr<Connection> &Session::GetConection() const
{
    return connection_;
}

bool Session::IsMonopolizeConection() const
{
    return connection_.unique();
}

void Session::ResetShutdownFlag()
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

void Session::DeleteObject()
{
    delete this;
}

void Session::PushRecvPacket(INetPacket *pck)
{
    if (IsActive()) {
        if (pck->GetOpcode() == OPCODE_LARGE_PACKET) {
            PushRecvFragmentPacket(pck);
        } else {
            recv_queue_.Enqueue(pck);
        }
        last_recv_pck_time_ = GET_SYS_TIME;
    } else {
        delete pck;
    }
}

void Session::PushSendPacket(const INetPacket &pck)
{
    if (IsActive()) {
        if (pck.GetReadableSize() > INetPacket::MAX_BUFFER_SIZE) {
            PushSendOverflowPacket(pck);
        } else {
            send_buffer_->WritePacket(pck);
        }
        connection_->PostWriteRequest();
        last_send_pck_time_ = GET_SYS_TIME;
    }
}

void Session::PushSendPacket(const char *data, size_t size)
{
    if (IsActive()) {
        send_buffer_->WritePacket(data, size);
        connection_->PostWriteRequest();
        last_send_pck_time_ = GET_SYS_TIME;
    }
}

void Session::PushSendPacket(const INetPacket &pck, const INetPacket &data)
{
    if (IsActive()) {
        send_buffer_->WritePacket(pck, data);
        connection_->PostWriteRequest();
        last_send_pck_time_ = GET_SYS_TIME;
    }
}

void Session::PushSendPacket(const INetPacket &pck, const char *data, size_t size)
{
    if (IsActive()) {
        send_buffer_->WritePacket(pck, data, size);
        connection_->PostWriteRequest();
        last_send_pck_time_ = GET_SYS_TIME;
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
    static void WritePacketHeader(INetPacket &pck, const INetPacket &data) {
        pck.Write<uint32>(data.GetReadableSize() + LARGE_PACKET_HEADER_SIZE);
        pck.Write<uint16>(data.GetOpcode());
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
            recv_queue_.Enqueue(&LargePacketHelper::UnpackPacket(*packet));
            fragment_packets_.erase(number);
        }
    } while (0);
}

void Session::PushSendOverflowPacket(const INetPacket &pck)
{
    auto send = [=](const INetPacket &pck, const INetPacket &data, size_t &offset)->bool {
        const size_t packet_space_size = INetPacket::MAX_BUFFER_SIZE - pck.GetTotalSize();
        const size_t length = std::min(data.GetTotalSize() - offset, packet_space_size);
        send_buffer_->WritePacket(pck, data.GetBuffer() + offset, length);
        offset += length;
        return length >= packet_space_size;
    };
    NetPacket packet(OPCODE_LARGE_PACKET);
    packet << (uint32)overflow_packet_count_.fetch_add(1);
    LargePacketHelper::WritePacketHeader(packet, pck);
    size_t offset = pck.GetReadPos();
    if (send(packet, pck, offset)) {
        packet.Shrink(packet.GetTotalSize() - LargePacketHelper::LARGE_PACKET_HEADER_SIZE);
        while (send(packet, pck, offset)) continue;
    }
}

void Session::ClearRecvPacket()
{
    INetPacket *pck = nullptr;
    while (recv_queue_.Swap()) {
        while (recv_queue_.Dequeue(pck)) {
            delete pck;
        }
    }
    do {
        std::lock_guard<std::mutex> lock(fragment_mutex_);
        for (auto &pair : fragment_packets_) {
            delete pair.second;
        }
        fragment_packets_.clear();
    } while (0);
}
