#include "network/ConnectionManager.h"
#include "network/SessionManager.h"
#include "network/Listener.h"
#include "udp/ConnectlessManager.h"
#include "udp/Receiver.h"
#include "udp/Sessionless.h"
#include "ServerMaster.h"
#include "Debugger.h"
#include "System.h"
#include "OS.h"

#define HOST_STRING "127.0.0.1"
#define PORT_STRING "9999"
#define sSessionlessServer (*SessionlessServer::instance())
#define sSessionlessReceiver (*SessionlessReceiver::instance())
#define sSessionlessListener (*SessionlessListener::instance())
#define sSessionlessServerMaster (*SessionlessServerMaster::instance())
#define sSessionlessConnectlessManager (*SessionlessConnectlessManager::instance())
#define sSessionlessConnectionManager (*SessionlessConnectionManager::instance())
#define sSessionlessSessionManager (*SessionlessSessionManager::instance())
#define sDataManager (*DataManager::instance())

class ProducerSession;

class SessionlessConnectlessManager : public ConnectlessManager, public Singleton<SessionlessConnectlessManager> {
};
class SessionlessConnectionManager : public ConnectionManager, public Singleton<SessionlessConnectionManager> {
};
class SessionlessSessionManager : public SessionManager, public Singleton<SessionlessSessionManager> {
};

class DataManager : public Singleton<DataManager> {
public:
    DataManager() {
        packet_string_.resize(65531);
        for (size_t i = 0, total = packet_string_.size(); i < total; ++i) {
            packet_string_[i] = System::Rand(0, 256);
        }
    }
    std::string packet_string_;
    std::atomic<int> send_pck_count_{0}, recv_pck_count_{0}, trans_pck_count_{0};
    std::unordered_set<ProducerSession*> producer_list_;
};

class ProducerSession : public Sessionless {
public:
    ProducerSession() : Sessionless(true) {}
    virtual ~ProducerSession() {
        for (; !queue_.empty(); queue_.pop()) {
            delete queue_.front();
        }
    }
    virtual int HandlePacket(INetPacket *pck) {
        switch (pck->GetOpcode()) {
            case 0: {
                std::string key = sSessionlessConnectlessManager.NewUniqueKey();
                SetConnectless(sSessionlessConnectlessManager.NewConnectless(key, sSessionlessConnectionManager.io_service(), *this));
                GetConnectless()->AsyncConnect(key, HOST_STRING, PORT_STRING);
                token_ = pck->Read<uint64>();
                break;
            }
            default: {
                std::unique_ptr<INetPacket> packet(queue_.front());
                DBGASSERT(packet->GetOpcode() == pck->GetOpcode());
                DBGASSERT(packet->GetReadableSize() == pck->GetReadableSize());
                DBGASSERT(memcmp(packet->GetReadableBuffer(), pck->GetReadableBuffer(), pck->GetReadableSize()) == 0);
                queue_.pop();
                sDataManager.recv_pck_count_.fetch_add(1);
                break;
            }
        }
        return SessionHandleSuccess;
    }
    void SendSomePacket() {
        if (!GetConnectless() || GetConnectless()->GetSendDataSize() > 65536 * 16 || queue_.size() > 256) {
            return;
        }
        for (int i = 0, total = System::Rand(1, 256); i < total; ++i) {
            const size_t offset = System::Rand(0, sDataManager.packet_string_.size());
            const size_t length = System::Rand(0, sDataManager.packet_string_.size() - offset + 1);
            INetPacket *pck = INetPacket::New(System::Rand(1, 60000), length);
            pck->Append(sDataManager.packet_string_.data() + offset, length);
            PushSendReliable(*pck);
            queue_.push(pck);
            sDataManager.send_pck_count_.fetch_add(1);
        }
    }
    static void New() {
        ProducerSession *session = new ProducerSession;
        session->SetConnection(sSessionlessConnectionManager.NewConnection(*session));
        session->GetConnection()->AsyncConnect(HOST_STRING, PORT_STRING);
        sSessionlessSessionManager.AddSession(session);
        sDataManager.producer_list_.insert(session);
    }
private:
    virtual void OnShutdownSession() {
        sDataManager.producer_list_.erase(this);
        if (!IServerMaster::GetInstance().IsEnd())
            New();
    }
    virtual void OnConnected() {
        NetPacket packet(0);
        PushSendPacket(packet);
    }
    virtual void OnReadyConnectless() {
        NetBuffer buffer;
        buffer << u8(0) << token_;
        for (size_t i = 0; i < 5; ++i) {
            PushSendBufferData(buffer.GetBuffer(), buffer.GetTotalSize());
            OS::SleepMS(100);
        }
    }
    std::queue<INetPacket*> queue_;
    uint64 token_;
};

class SessionlessSession : public Sessionless {
public:
    SessionlessSession() : Sessionless(true) {}
    virtual int HandlePacket(INetPacket *pck) {
        switch (pck->GetOpcode()) {
            case 0: {
                NetPacket packet(0);
                packet << reinterpret_cast<uint64>(this);
                PushSendPacket(packet);
                break;
            }
            default: {
                PushSendReliable(*pck);
                sDataManager.trans_pck_count_.fetch_add(1);
                break;
            }
        }
        return SessionHandleSuccess;
    }
};

class SessionlessReceiver : public Receiver, public Singleton<SessionlessReceiver> {
public:
    virtual ConnectlessManager *GetConnectlessManager() { return &sSessionlessConnectlessManager; }
    virtual asio::io_service *GetAsioServeice() { return &sSessionlessConnectionManager.io_service(); }
    virtual std::string GetBindAddress() { return HOST_STRING; }
    virtual std::string GetBindPort() { return PORT_STRING; }
    virtual Sessionless *NewSessionlessObject(ConstNetBuffer &buffer) {
        return reinterpret_cast<Sessionless*>(buffer.Read<uint64>());
    }
};

class SessionlessListener : public Listener, public Singleton<SessionlessListener> {
public:
    virtual ConnectionManager *GetConnectionManager() { return &sSessionlessConnectionManager; }
    virtual SessionManager *GetSessionManager() { return &sSessionlessSessionManager; }
    virtual std::string GetBindAddress() { return HOST_STRING; }
    virtual std::string GetBindPort() { return PORT_STRING; }
    virtual Session *NewSessionObject() { return new SessionlessSession(); }
};

class SessionlessServer : public Thread, public Singleton<SessionlessServer> {
public:
    virtual bool Initialize() {
        for (int i = 0; i < 5; ++i) {
            ProducerSession::New();
            OS::SleepMS(1);
        }
        return true;
    }
    virtual void Abort() {
    }
    virtual void Kernel() {
        for (auto session : sDataManager.producer_list_) {
            if (session->IsActive()) {
                session->SendSomePacket();
            }
        }
        sSessionlessSessionManager.Update();
        printf("[%d] --- %d <---> %d\r", sDataManager.trans_pck_count_.load(),
            sDataManager.send_pck_count_.load(), sDataManager.recv_pck_count_.load());
        System::Update();
        OS::SleepMS(1);
    }
};

class SessionlessServerMaster : public IServerMaster, public Singleton<SessionlessServerMaster> {
public:
    virtual bool InitSingleton() {
        DataManager::newInstance();
        SessionlessServer::newInstance();
        SessionlessReceiver::newInstance();
        SessionlessConnectlessManager::newInstance();
        SessionlessListener::newInstance();
        SessionlessConnectionManager::newInstance();
        SessionlessSessionManager::newInstance();
        return true;
    }
    virtual void FinishSingleton() {
        SessionlessSessionManager::deleteInstance();
        SessionlessConnectionManager::deleteInstance();
        SessionlessListener::deleteInstance();
        SessionlessConnectlessManager::deleteInstance();
        SessionlessReceiver::deleteInstance();
        SessionlessServer::deleteInstance();
        DataManager::deleteInstance();
    }
protected:
    virtual bool InitDBPool() { return true; }
    virtual bool LoadDBData() { return true; }
    virtual bool StartServices() {
        sSessionlessConnectionManager.SetWorkerCount(3);
        sSessionlessConnectionManager.Start();
        sSessionlessListener.Start();
        sSessionlessConnectlessManager.Start();
        sSessionlessReceiver.Start();
        sSessionlessServer.Start();
        return true;
    }
    virtual void StopServices() {
        sSessionlessReceiver.Stop();
        sSessionlessConnectlessManager.Stop();
        sSessionlessSessionManager.Stop();
        sSessionlessConnectionManager.Stop();
        sSessionlessListener.Stop();
        sSessionlessServer.Stop();
    }
    virtual void Tick() {}
    virtual std::string GetConfigFile() { return "config"; }
};

void SessionlessMain(int argc, char **argv)
{
    SessionlessServerMaster::newInstance();
    sSessionlessServerMaster.InitSingleton();
    sSessionlessServerMaster.Initialize(argc, argv);
    sSessionlessServerMaster.Run(argc, argv);
    sSessionlessServerMaster.FinishSingleton();
    SessionlessServerMaster::deleteInstance();
}
