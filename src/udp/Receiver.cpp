#include "Receiver.h"
#include "ConnectlessManager.h"
#include "network/IOServiceManager.h"
#include "Logger.h"
#include "OS.h"

Receiver::Receiver()
: sockfd_(INVALID_SOCKET)
{
}

Receiver::~Receiver()
{
}

bool Receiver::Prepare()
{
    addr_ = GetBindAddress();
    port_ = GetBindPort();
    return true;
}

bool Receiver::Initialize()
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    int ret = getaddrinfo(addr_.c_str(), port_.c_str(), &hints, &res);
    if (ret != 0) {
        ELOG("getaddrinfo(), error: %s.", gai_strerror(ret));
        return false;
    }

    std::unique_ptr<addrinfo, decltype(freeaddrinfo)*> _(res, freeaddrinfo);
    sockfd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd_ == INVALID_SOCKET) {
        ELOG("socket(), errno: %d.", GET_SOCKET_ERROR());
        return false;
    }

    if (!OS::non_blocking(sockfd_)) {
        ELOG("non_blocking(), errno: %d.", GET_SOCKET_ERROR());
        return false;
    }

    if (!OS::reuse_address(sockfd_)) {
        ELOG("reuse_address(), errno: %d.", GET_SOCKET_ERROR());
        return false;
    }

    ret = bind(sockfd_, res->ai_addr, res->ai_addrlen);
    if (ret != 0) {
        ELOG("bind(), errno: %d.", GET_SOCKET_ERROR());
        return false;
    }

    auto protocol = res->ai_family == AF_INET ?
        boost::asio::ip::udp::v4() : boost::asio::ip::udp::v6();
    socket_ = std::make_shared<boost::asio::ip::udp::socket>(
        sIOServiceManager.SelectWorkerLoadLowest(), protocol, sockfd_);

    return true;
}

void Receiver::Kernel()
{
    struct pollfd sockfd;
    sockfd.fd = sockfd_;
    sockfd.events = POLLRDNORM;
    int ret = poll(&sockfd, 1, 100);
    if (ret == SOCKET_ERROR) {
        ELOG("poll(), errno: %d.", GET_SOCKET_ERROR());
        return;
    }

    if (ret == 0 || (sockfd.revents & POLLRDNORM) == 0) {
        return;
    }

    while (true) {
        char buffer[UDP_PKT_MAX+100];
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);
        ssize_t len = recvfrom(sockfd_,
            buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen);
        if (len >= 0) {
            OnRecvPkt(addr, buffer, len);
        } else {
            if (GET_SOCKET_ERROR() != ERROR_WOULDBLOCK) {
                ELOG("recvfrom(), errno: %d.", GET_SOCKET_ERROR());
            }
            break;
        }
    }
}

void Receiver::Finish()
{
    if (!socket_ && sockfd_ != INVALID_SOCKET) {
        closesocket(sockfd_);
        sockfd_ = INVALID_SOCKET;
    }
    if (socket_) {
        socket_->close();
        socket_.reset();
    }
}

void Receiver::OnRecvPkt(const struct sockaddr_storage &addr, const char *data, size_t size)
{
    ConstNetBuffer buffer(data, size);
    const uint8 type = buffer.Read<u8>();

    std::string key = CalcKey(addr);
    std::shared_ptr<Connectless> conn = sConnectlessManager.FindConnectless(key);
    if (conn) {
        if (conn->IsActive()) {
            conn->OnRecvPkt(type, buffer);
        }
        return;
    }

    if (type == 0) {
        auto sessionless = NewSessionlessObject(buffer);
        if (sessionless != nullptr) {
            std::shared_ptr<Connectless> conn =
                sConnectlessManager.NewConnectless(key, *sessionless);
            conn->SetSocket(key, socket_, Address(addr));
        }
    }
}

std::string Receiver::CalcKey(const struct sockaddr_storage &addr)
{
    std::string key; key.reserve(16+2);
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in &sin = (struct sockaddr_in &)addr;
        key.append((const char *)(&sin.sin_addr), sizeof(sin.sin_addr));
        key.append((const char *)(&sin.sin_port), sizeof(sin.sin_port));
    } else {
        struct sockaddr_in6 &sin6 = (struct sockaddr_in6 &)addr;
        key.append((const char *)(&sin6.sin6_addr), sizeof(sin6.sin6_addr));
        key.append((const char *)(&sin6.sin6_port), sizeof(sin6.sin6_port));
    }
    return key;
}

boost::asio::ip::udp::socket::endpoint_type Receiver::Address(const struct sockaddr_storage &addr)
{
    boost::asio::ip::udp::socket::endpoint_type address;
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in &sin = (struct sockaddr_in &)addr;
        address.address(boost::asio::ip::address_v4(ntohl(sin.sin_addr.s_addr)));
        address.port(ntohs(sin.sin_port));
    } else {
        struct sockaddr_in6 &sin6 = (struct sockaddr_in6 &)addr;
        address.address(boost::asio::ip::address_v6((boost::asio::ip::address_v6::bytes_type&)
            sin6.sin6_addr.s6_addr, sin6.sin6_scope_id));
        address.port(ntohs(sin6.sin6_port));
    }
    return address;
}
