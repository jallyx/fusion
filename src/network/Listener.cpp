#include "Listener.h"
#include "ConnectionManager.h"
#include "SessionManager.h"
#include "Logger.h"
#include "OS.h"

Listener::Listener()
: sockfd_(INVALID_SOCKET)
{
}

Listener::~Listener()
{
}

bool Listener::Prepare()
{
    addr_ = GetBindAddress();
    port_ = GetBindPort();
    return true;
}

bool Listener::Initialize()
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
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

    ret = listen(sockfd_, 5);
    if (ret != 0) {
        ELOG("listen(), errno: %d.", GET_SOCKET_ERROR());
        return false;
    }

    return true;
}

void Listener::Kernel()
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
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);
        SOCKET sockfd = accept(sockfd_, (struct sockaddr *)&addr, &addrlen);
        if (sockfd != INVALID_SOCKET) {
            OnAcceptComplete(addr.ss_family, sockfd);
        } else {
            if (GET_SOCKET_ERROR() != ERROR_WOULDBLOCK) {
                ELOG("accept(), errno: %d.", GET_SOCKET_ERROR());
            }
            break;
        }
    }
}

void Listener::Finish()
{
    if (sockfd_ != INVALID_SOCKET) {
        closesocket(sockfd_);
        sockfd_ = INVALID_SOCKET;
    }
}

void Listener::OnAcceptComplete(int family, SOCKET sockfd)
{
    Session *session = NewSessionObject();
    std::shared_ptr<Connection> connPtr = sConnectionManager.NewConnection(*session);

    const boost::asio::ip::tcp::socket::protocol_type protocol =
        family == AF_INET ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6();
    connPtr->SetSocket(protocol, sockfd);

    sSessionManager.AddSession(session);
    connPtr->PostReadRequest();
}
