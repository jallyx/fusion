#include "Connection.h"
#include "ConnectionManager.h"
#include "Session.h"
#include "System.h"
#include "Logger.h"

Connection::Connection(boost::asio::io_service &io_service,
    ConnectionManager &manager, Session &session, int load_value)
: load_value_(load_value)
, manager_(manager)
, session_(session)
, is_active_(false)
, resolver_(io_service)
, sock_(io_service)
, port_(0)
, is_connected_(false)
, recv_buffer_(MAX_NET_PACKET_SIZE+1)
, deflate_stream_(nullptr)
, inflate_stream_(nullptr)
, final_send_buffer_(nullptr)
, final_recv_buffer_(nullptr)
, is_residual_send_data_(false)
, is_reading_{ATOMIC_FLAG_INIT}
, is_writing_{ATOMIC_FLAG_INIT}
, last_recv_data_time_(GET_APP_TIME)
, last_send_data_time_(GET_APP_TIME)
{
}

Connection::~Connection()
{
    Close();
    SAFE_DELETE(deflate_stream_);
    SAFE_DELETE(inflate_stream_);
    SAFE_DELETE(final_send_buffer_);
    SAFE_DELETE(final_recv_buffer_);
}

bool Connection::HasSendDataAwaiting() const
{
    if (final_send_buffer_ && !final_send_buffer_->IsEmpty())
        return true;
    if (send_buffer_.HasSendDataAvail())
        return true;
    return false;
}

size_t Connection::GetSendDataSize() const
{
    return (final_send_buffer_ ? final_send_buffer_->GetSafeDataSize() : 0) +
        send_buffer_.GetDataSize();
}

void Connection::Init()
{
    if (session_.is_deflate_packet()) {
        deflate_stream_ = new zlib::DeflateStream();
        final_send_buffer_ = new CircularBuffer(MAX_NET_PACKET_SIZE+1);
    }
    if (session_.is_inflate_packet()) {
        inflate_stream_ = new zlib::InflateStream();
        final_recv_buffer_ = new CircularBuffer(MAX_NET_PACKET_SIZE+1);
    }

    session_.SetConnection(shared_from_this());
}

void Connection::Close()
{
    if (is_active_) {
        is_active_ = is_connected_ = false;
        WLOG("Close connection from [%s:%hu].", addr_.c_str(), port_);

        manager_.RemoveConnection(shared_from_this());
        if (session_.IsActive()) {
            session_.KillSession();
        }

        sock_.close();
    }
}

void Connection::SetSocket(const boost::asio::ip::tcp::socket::protocol_type &protocol, SOCKET socket)
{
    is_active_ = is_connected_ = true;
    sock_.assign(protocol, socket);
    sock_.non_blocking(true);
    sock_.set_option(boost::asio::ip::tcp::no_delay(true));
    RenewRemoteEndpoint();
}

void Connection::AsyncConnect(const std::string &address, const std::string &port)
{
    is_active_ = true;
    addr_ = address;
    port_ = atoi(port.c_str());

    boost::asio::ip::tcp::resolver::query query(address, port);
    resolver_.async_resolve(query,
        std::bind(&Connection::OnResolveComplete, shared_from_this(),
                  std::placeholders::_1, std::placeholders::_2));
}

void Connection::PostReadRequest()
{
    if (!is_reading_.test_and_set()) {
        sock_.get_io_service().post(
            std::bind(&Connection::StartNextRead, shared_from_this()));
    }
}

void Connection::PostWriteRequest()
{
    if (IsConnected() && !is_writing_.test_and_set()) {
        sock_.get_io_service().post(
            std::bind(&Connection::StartNextWrite, shared_from_this()));
    }
}

void Connection::PostCloseRequest()
{
    if (IsActive()) {
        sock_.get_io_service().post(
            std::bind(&Connection::Close, shared_from_this()));
    }
}

void Connection::StartNextRead()
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        size_t size = 0;
        char *buffer = GetRecvDataBuffer(size);
        sock_.async_read_some(boost::asio::buffer(buffer, size),
            std::bind(&Connection::OnReadComplete, shared_from_this(),
                      std::placeholders::_1, buffer, std::placeholders::_2));

    } TRY_END
    CATCH_BEGIN(const boost::system::system_error &e) {
        WLOG("StartNextRead[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("StartNextRead[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("StartNextRead[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connection::StartNextWrite()
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        size_t size = 0;
        const char *buffer = PreprocessAndGetSendDataBuffer(size);
        if (buffer != nullptr && size != 0) {
            sock_.async_write_some(boost::asio::buffer(buffer, size),
                std::bind(&Connection::OnWriteComplete, shared_from_this(),
                          std::placeholders::_1, buffer, std::placeholders::_2));
        } else {
            is_writing_.clear();
            if (HasSendDataAwaiting() && !is_writing_.test_and_set()) {
                StartNextWrite();
            }
        }

    } TRY_END
    CATCH_BEGIN(const boost::system::system_error &e) {
        WLOG("StartNextWrite[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("StartNextWrite[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("StartNextWrite[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connection::OnResolveComplete(const boost::system::error_code &ec, boost::asio::ip::tcp::resolver::iterator itr)
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        if (ec) {
            WLOG("Resolve connection[%s:%hu], %s.", addr_.c_str(), port_, ec.message().c_str());
            Close();
            return;
        }

        last_recv_data_time_ = GET_APP_TIME;
        last_send_data_time_ = GET_APP_TIME;
        sock_.async_connect(*itr,
            std::bind(&Connection::OnConnectComplete, shared_from_this(),
                      std::placeholders::_1));

    } TRY_END
    CATCH_BEGIN(const boost::system::system_error &e) {
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

void Connection::OnConnectComplete(const boost::system::error_code &ec)
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        if (ec) {
            WLOG("Connect connection[%s:%hu], %s.", addr_.c_str(), port_, ec.message().c_str());
            Close();
            return;
        }

        last_recv_data_time_ = GET_APP_TIME;
        last_send_data_time_ = GET_APP_TIME;
        is_connected_ = true;
        sock_.non_blocking(true);
        sock_.set_option(boost::asio::ip::tcp::no_delay(true));
        PostReadRequest();
        PostWriteRequest();
        session_.OnConnected();

    } TRY_END
    CATCH_BEGIN(const boost::system::system_error &e) {
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

void Connection::OnReadComplete(const boost::system::error_code &ec, const char *buffer, std::size_t bytes)
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        if (ec) {
            WLOG("Read connection[%s:%hu], %s.", addr_.c_str(), port_, ec.message().c_str());
            Close();
            return;
        }

        last_recv_data_time_ = GET_APP_TIME;
        OnRecvDataCallback(buffer, bytes);
        StartNextRead();

    } TRY_END
    CATCH_BEGIN(const boost::system::system_error &e) {
        WLOG("OnReadComplete[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("OnReadComplete[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("OnReadComplete[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

void Connection::OnWriteComplete(const boost::system::error_code &ec, const char *buffer, std::size_t bytes)
{
    TRY_BEGIN {

        if (!IsActive()) {
            return;
        }

        if (ec) {
            WLOG("Write connection[%s:%hu], %s.", addr_.c_str(), port_, ec.message().c_str());
            Close();
            return;
        }

        last_send_data_time_ = GET_APP_TIME;
        OnSendDataCallback(buffer, bytes);
        StartNextWrite();

    } TRY_END
    CATCH_BEGIN(const boost::system::system_error &e) {
        WLOG("OnWriteComplete[%s:%hu] exception[%s] occurred.", addr_.c_str(), port_, e.what());
        Close();
    } CATCH_END
    CATCH_BEGIN(const IException &e) {
        WLOG("OnWriteComplete[%s:%hu] exception occurred.", addr_.c_str(), port_);
        e.Print();
        Close();
    } CATCH_END
    CATCH_BEGIN(...) {
        WLOG("OnWriteComplete[%s:%hu] unknown exception occurred.", addr_.c_str(), port_);
        Close();
    } CATCH_END
}

CircularBuffer &Connection::GetFinalRecvBuffer()
{
    return final_recv_buffer_ ? *final_recv_buffer_ : recv_buffer_;
}

INetPacket *Connection::ReadPacketFromBuffer()
{
    CircularBuffer &recvBuffer = GetFinalRecvBuffer();
    if (recvBuffer.GetReadableSpace() < INetPacket::Header::SIZE) {
        return nullptr;
    }

    INetPacket::Header header;
    TNetPacket<INetPacket::Header::SIZE> wrapper;
    wrapper.Erlarge(INetPacket::Header::SIZE);
    recvBuffer.Peek((char*)wrapper.GetBuffer(), wrapper.GetTotalSize());
    wrapper.ReadHeader(header);
    if (recvBuffer.GetReadableSpace() < header.len) {
        return nullptr;
    }

    INetPacket *pck = INetPacket::New(header.cmd, header.len - INetPacket::Header::SIZE);
    pck->Erlarge(header.len - INetPacket::Header::SIZE);
    recvBuffer.Remove(INetPacket::Header::SIZE);
    recvBuffer.Read((char*)pck->GetBuffer(), pck->GetTotalSize());
    return pck;
}

char *Connection::GetRecvDataBuffer(size_t &size)
{
    size = recv_buffer_.GetContiguiousWritableSpace();
    return recv_buffer_.GetContiguiousWritableBuffer();
}

const char *Connection::GetSendDataBuffer(size_t &size)
{
    if (final_send_buffer_ != nullptr) {
        size = final_send_buffer_->GetContiguiousReadableSpace();
        return final_send_buffer_->GetContiguiousReadableBuffer();
    } else {
        return send_buffer_.GetSendDataBuffer(size);
    }
}

const char *Connection::PreprocessAndGetSendDataBuffer(size_t &size)
{
    if (session_.is_deflate_packet()) {
        PreprocessSendData();
    }

    return GetSendDataBuffer(size);
}

void Connection::OnRecvDataCallback(const char *buffer, size_t size)
{
    size_t sizeCheck = 0;
    const char *bufferCheck = GetRecvDataBuffer(sizeCheck);
    if (buffer != bufferCheck || size > sizeCheck) {
        THROW_EXCEPTION(RecvDataException());
    }

    recv_buffer_.IncrementContiguiousWritten(size);

    do {
        if (session_.is_inflate_packet()) {
            PreprocessRecvData();
        }
        while (IsActive()) {
            INetPacket *pck = ReadPacketFromBuffer();
            if (pck != nullptr) {
                session_.PushRecvPacket(pck);
            } else {
                break;
            }
        }
        if (!session_.is_inflate_packet()) {
            break;
        }
    } while (IsActive() && !recv_buffer_.IsEmpty());
}

void Connection::OnSendDataCallback(const char *buffer, size_t size)
{
    size_t sizeCheck = 0;
    const char *bufferCheck = GetSendDataBuffer(sizeCheck);
    if (buffer != bufferCheck || size > sizeCheck) {
        THROW_EXCEPTION(SendDataException());
    }

    if (final_send_buffer_ != nullptr) {
        final_send_buffer_->IncrementContiguiousRead(size);
    } else {
        send_buffer_.RemoveSendData(size);
    }
}

void Connection::PreprocessRecvData()
{
    while (IsActive() && !recv_buffer_.IsEmpty() && !final_recv_buffer_->IsFull()) {
        size_t inlen = recv_buffer_.GetContiguiousReadableSpace();
        const char *in = recv_buffer_.GetContiguiousReadableBuffer();
        size_t outlen = final_recv_buffer_->GetContiguiousWritableSpace();
        char *out = final_recv_buffer_->GetContiguiousWritableBuffer();
        if (inflate_stream_->Inflate(in, inlen, out, outlen)) {
            if (inlen != 0) {
                recv_buffer_.IncrementContiguiousRead(inlen);
            }
            if (outlen != 0) {
                final_recv_buffer_->IncrementContiguiousWritten(outlen);
            }
        } else {
            THROW_EXCEPTION(RecvDataException());
        }
    }
}

void Connection::PreprocessSendData()
{
    while (IsActive() && send_buffer_.HasSendDataAvail() && !final_send_buffer_->IsFull()) {
        size_t inlen = 0;
        const char *in = send_buffer_.GetSendDataBuffer(inlen);
        size_t outlen = final_send_buffer_->GetContiguiousWritableSpace();
        char *out = final_send_buffer_->GetContiguiousWritableBuffer();
        if (deflate_stream_->Deflate(in, inlen, out, outlen)) {
            if (inlen != 0) {
                send_buffer_.RemoveSendData(inlen);
            }
            if (outlen != 0) {
                final_send_buffer_->IncrementContiguiousWritten(outlen);
            }
            is_residual_send_data_ = true;
        } else {
            THROW_EXCEPTION(SendDataException());
        }
    }

    while (is_residual_send_data_ && IsActive() && !final_send_buffer_->IsFull()) {
        size_t availen = final_send_buffer_->GetContiguiousWritableSpace();
        char *out = final_send_buffer_->GetContiguiousWritableBuffer();
        auto outlen = availen;
        if (deflate_stream_->Flush(out, outlen)) {
            if (outlen != 0) {
                final_send_buffer_->IncrementContiguiousWritten(outlen);
            }
            if (outlen < availen) {
                is_residual_send_data_ = false;
            }
        } else {
            THROW_EXCEPTION(SendDataException());
        }
    }
}

void Connection::RenewRemoteEndpoint()
{
    boost::system::error_code ec;
    boost::asio::ip::tcp::endpoint endpoint = sock_.remote_endpoint(ec);
    if (!ec) {
        addr_ = endpoint.address().to_string();
        port_ = endpoint.port();
    }
}

void Connection::InitSendBufferPool()
{
    SendBuffer::InitBufferPool();
}

void Connection::ClearSendBufferPool()
{
    SendBuffer::ClearBufferPool();
}
