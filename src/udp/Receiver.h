#pragma once

#include "Thread.h"
#include "Macro.h"
#include "NetBuffer.h"
#include "network/AsioHeader.h"

class ConnectlessManager;
class Sessionless;

class Receiver : public Thread
{
public:
    THREAD_RUNTIME(Receiver)

    Receiver();
    virtual ~Receiver();

protected:
    virtual bool Prepare();
    virtual bool Initialize();
    virtual void Kernel();
    virtual void Finish();

    virtual ConnectlessManager *GetConnectlessManager() = 0;
    virtual asio::io_service *GetAsioServeice() = 0;
    virtual std::string GetBindAddress() = 0;
    virtual std::string GetBindPort() = 0;

    virtual Sessionless *NewSessionlessObject(ConstNetBuffer &buffer) = 0;

private:
    void OnRecvPkt(const struct sockaddr_storage &addr, const char *data, size_t size);

    static std::string CalcKey(const struct sockaddr_storage &addr);
    static asio::ip::udp::socket::endpoint_type Address(const struct sockaddr_storage &addr);

    SOCKET sockfd_;
    std::shared_ptr<asio::ip::udp::socket> socket_;

    ConnectlessManager *connectless_manager_;
    asio::io_service *io_service_;
    std::string addr_;
    std::string port_;
};
