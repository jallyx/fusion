#pragma once

#include "RecvQueue.h"
#include "SendQueue.h"
#include "NetBuffer.h"
#include "network/AsioHeader.h"
#include "network/CircularBuffer.h"
#include "network/SendBuffer.h"
#include "zlib/ZlibStream.h"

class ConnectlessManager;
class Sessionless;

class Connectless : public std::enable_shared_from_this<Connectless>
{
public:
    Connectless(boost::asio::io_service &io_service,
        ConnectlessManager &manager, Sessionless &sessionless, int load_value);
    ~Connectless();

    void Init();

    void PostCloseRequest();

    void SetSocket(const std::string &key,
        const std::shared_ptr<boost::asio::ip::udp::socket> &socket,
        const boost::asio::ip::udp::socket::endpoint_type &destination);
    void AsyncConnect(const std::string &key,
        const std::string &address, const std::string &port);

    void SendPacketReliable(const INetPacket &pck);
    void SendPacketUnreliable(const INetPacket &pck);
    void SendBufferData(const void *data, size_t size);

    void OnRecvPkt(uint8 type, ConstNetBuffer &buffer);

    bool HasSendDataAwaiting() const;
    size_t GetSendDataSize() const;

    bool IsActive() const { return is_active_; }
    bool IsConnected() const { return is_connected_; }

    const std::string &key() const { return key_; }

    int get_load_value() const { return load_value_; }
    boost::asio::io_service &get_io_service() { return resolver_.get_io_service(); }

    static void InitQueueBufferPool();
    static void ClearQueueBufferPool();

private:
    void Close();

    void PostRecvRequest();
    void PostReadRequest();
    void PostWriteRequest();
    void PostPacketSendRequest();

    void StartNextRecv();
    void StartDataRead();
    void StartDataWrite();
    void StartPacketSend();

    void SendAck(uint8 age, uint64 seq, uint64 una);
    void OnPktAck(uint8 age, uint64 seq, uint64 una);

    void OnResolveComplete(const boost::system::error_code &ec, boost::asio::ip::udp::resolver::iterator itr);
    void OnConnectComplete(const boost::system::error_code &ec);
    void OnRecvComplete(const boost::system::error_code &ec, std::size_t bytes);

    void PostPktRtoWriteRequest(uint32 expiry);
    void PostPktLostWriteRequest(uint32 expiry);

    void OnPktRtoWrite(const boost::system::error_code &ec, uint32 expiry);
    void OnPktLostWrite(const boost::system::error_code &ec, uint32 expires);

    void CleanPktLostInfos();

    void PreprocessRecvData();
    void PreprocessSendData();

    void RefreshRTO(uint32 rtt);
    void UpsideRTO();
    uint32 rto() const;

    class MultiBuffers {
    public:
        MultiBuffers() : n_(0) {}
        typedef boost::asio::const_buffer value_type;
        typedef const boost::asio::const_buffer *const_iterator;
        const_iterator begin() const { return buffs_; }
        const_iterator end() const { return buffs_ + n_; }
        void add(const SendQueue::PktValue &buffers) {
            for (const auto &buffer : buffers) {
                add(buffer.str, buffer.len);
            }
        }
        void add(const void *data, size_t size) {
            buffs_[n_++] = value_type{ data, size };
            assert(n_ <= ARRAY_SIZE(buffs_));
        }
    private:
        boost::asio::const_buffer buffs_[3];
        size_t n_;
    };
    void SendPkt(const MultiBuffers &buffers);
    void SendPkt(const SendQueue::PktValue &pkt);

    struct PacketInfo {
        uint32 sn[2];
        INetPacket *pck;
        bool operator<(const PacketInfo &other) const {
            if (sn[0] < other.sn[0]) return true;
            if (sn[0] > other.sn[0]) return false;
            if (sn[1] < other.sn[1]) return true;
            if (sn[1] > other.sn[1]) return false;
            return false;
        }
    };
    PacketInfo ReadPacketReliableFromBuffer();
    bool ReadPacketUnreliableFromBuffer();
    void DispatchPacketUnreliable(const PacketInfo &info);
    void DispatchPacket(INetPacket *pck);

    const int load_value_;

    ConnectlessManager &manager_;
    Sessionless &sessionless_;
    bool is_active_;

    boost::asio::ip::udp::resolver resolver_;
    std::shared_ptr<boost::asio::ip::udp::socket> sock_;
    boost::asio::ip::udp::socket::endpoint_type dest_;
    std::string key_, addr_;
    unsigned short port_;
    bool is_connected_;

    SendBuffer send_buffer_;
    RecvQueue recv_queue_;

    zlib::DeflateStream deflate_stream_;
    zlib::InflateStream inflate_stream_;
    SendQueue final_send_queue_;
    CircularBuffer final_recv_buffer_;
    char async_recv_buffer[UDP_PKT_MAX+100];
    bool is_residual_send_data_;

    SendBuffer packet_buffer_;
    TNetPacket<UDP_PKT_MAX+100> packet_cache_;

    float srtt_;
    float rttvar_;

    uint32 recv_sn_[2];
    std::atomic<uint32> send_sn_[2];
    std::priority_queue<PacketInfo> packets_;

    boost::asio::deadline_timer rto_timer_;
    boost::asio::deadline_timer lost_timer_;
    std::deque<std::pair<uint32, uint32>> lost_infos_;

    std::atomic_flag is_recving_, is_sending_;
    std::atomic_flag is_reading_, is_writing_;
    uint64 last_recv_data_time_, last_send_data_time_;
};
