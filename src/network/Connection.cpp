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
, first_send_pipe_(nullptr)
, send_pipe_(nullptr)
, recv_pipe_(nullptr)
, is_reading_{ATOMIC_FLAG_INIT}
, is_writing_{ATOMIC_FLAG_INIT}
, last_recv_data_time_(GET_APP_TIME)
, last_send_data_time_(GET_APP_TIME)
{
    send_pipe_ = first_send_pipe_ = new SendDataFirstPipe(is_active_);
    recv_pipe_ = new RecvDataLastPipe(std::bind(&Session::PushRecvPacket,
        std::ref(session), std::placeholders::_1), is_active_);
}

Connection::~Connection()
{
    Close();
    SAFE_DELETE(send_pipe_);
    SAFE_DELETE(recv_pipe_);
}

void Connection::Init()
{
    session_.SetConnection(shared_from_this());
}

void Connection::AddDataPipe(ISendDataPipe *send_pipe, IRecvDataPipe *recv_pipe)
{
    if (send_pipe != nullptr) {
        send_pipe->Init(send_pipe_);
        send_pipe_ = send_pipe;
    }
    if (recv_pipe != nullptr) {
        recv_pipe->Init(recv_pipe_);
        recv_pipe_ = recv_pipe;
    }
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
        char *buffer = recv_pipe_->GetRecvDataBuffer(size);
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
        const char *buffer = send_pipe_->GetSendDataBuffer(size);
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

void Connection::OnRecvDataCallback(const char *buffer, size_t size)
{
    size_t sizeCheck = 0;
    const char *bufferCheck = recv_pipe_->GetRecvDataBuffer(sizeCheck);
    if (buffer != bufferCheck || size > sizeCheck) {
        THROW_EXCEPTION(RecvDataException());
    }

    recv_pipe_->IncrementRecvData(size);
}

void Connection::OnSendDataCallback(const char *buffer, size_t size)
{
    size_t sizeCheck = 0;
    const char *bufferCheck = send_pipe_->GetSendDataBuffer(sizeCheck);
    if (buffer != bufferCheck || size > sizeCheck) {
        THROW_EXCEPTION(SendDataException());
    }

    send_pipe_->RemoveSendData(size);
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
