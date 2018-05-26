#pragma once

#include "AsioHeader.h"
#include "CircularBuffer.h"
#include "SendBuffer.h"
#include "zlib/ZlibStream.h"

class ConnectionManager;
class Session;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    Connection(boost::asio::io_service &io_service,
        ConnectionManager &manager, Session &session, int load_value);
    ~Connection();

    void Init();

    void PostReadRequest();
    void PostWriteRequest();
    void PostCloseRequest();

    void SetSocket(const boost::asio::ip::tcp::socket::protocol_type &protocol, SOCKET socket);
    void AsyncConnect(const std::string &address, const std::string &port);

    bool HasSendDataAwaiting() const;
    size_t GetSendDataSize() const;

    bool IsActive() const { return is_active_; }
    bool IsConnected() const { return is_connected_; }

    const std::string &addr() const { return addr_; }
    unsigned short port() const { return port_; }

    int get_load_value() const { return load_value_; }
    boost::asio::io_service &get_io_service() { return sock_.get_io_service(); }

    SendBuffer &GetSendBuffer() { return send_buffer_; }

    static void InitSendBufferPool();
    static void ClearSendBufferPool();

private:
    void Close();

    void StartNextRead();
    void StartNextWrite();

    void OnResolveComplete(const boost::system::error_code &ec, boost::asio::ip::tcp::resolver::iterator itr);
    void OnConnectComplete(const boost::system::error_code &ec);
    void OnReadComplete(const boost::system::error_code &ec, const char *buffer, std::size_t bytes);
    void OnWriteComplete(const boost::system::error_code &ec, const char *buffer, std::size_t bytes);

    char *GetRecvDataBuffer(size_t &size);
    const char *GetSendDataBuffer(size_t &size);
    const char *PreprocessAndGetSendDataBuffer(size_t &size);
    void OnRecvDataCallback(const char *buffer, size_t size);
    void OnSendDataCallback(const char *buffer, size_t size);

    void PreprocessRecvData();
    void PreprocessSendData();

    void RenewRemoteEndpoint();

    CircularBuffer &GetFinalRecvBuffer();
    INetPacket *ReadPacketFromBuffer();

    const int load_value_;

    ConnectionManager &manager_;
    Session &session_;
    bool is_active_;

    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket sock_;
    std::string addr_;
    unsigned short port_;
    bool is_connected_;

    SendBuffer send_buffer_;
    CircularBuffer recv_buffer_;

    zlib::DeflateStream *deflate_stream_;
    zlib::InflateStream *inflate_stream_;
    CircularBuffer *final_send_buffer_;
    CircularBuffer *final_recv_buffer_;
    bool is_residual_send_data_;

    std::atomic_flag is_reading_, is_writing_;
    uint64 last_recv_data_time_, last_send_data_time_;
};
