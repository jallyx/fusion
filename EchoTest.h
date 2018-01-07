#include "network/ConnectionManager.h"
#include "network/SessionManager.h"
#include "network/Listener.h"
#include "ServerMaster.h"
#include "Debugger.h"
#include "OS.h"

#define HOST_STRING "127.0.0.1"
#define PORT_STRING "9999"
#define sEchoServer (*EchoServer::instance())
#define sEchoListener (*EchoListener::instance())
#define sEchoServerMaster (*EchoServerMaster::instance())
#define sEchoConnectionManager (*EchoConnectionManager::instance())
#define sEchoSessionManager (*EchoSessionManager::instance())
#define sDataManager (*DataManager::instance())

class ProducerSession;

class EchoConnectionManager : public ConnectionManager, public Singleton<EchoConnectionManager> {
};
class EchoSessionManager : public SessionManager, public Singleton<EchoSessionManager> {
};

class DataManager : public Singleton<DataManager> {
public:
    DataManager() {
        packet_string_.resize(65536 * 5);
        for (size_t i = 0, total = packet_string_.size(); i < total; ++i) {
            packet_string_[i] = System::Rand(0, 256);
        }
    }
    std::string packet_string_;
    std::atomic<int> send_pck_count_{0}, recv_pck_count_{0}, trans_pck_count_{0};
    std::unordered_set<ProducerSession*> producer_list_;
};

class ProducerSession : public Session {
public:
    ProducerSession() : Session(true, true, true) {}
    virtual ~ProducerSession() {
        for (; !queue_.empty(); queue_.pop()) {
            delete queue_.front();
        }
    }
    virtual int HandlePacket(INetPacket *pck) {
        std::unique_ptr<INetPacket> packet(queue_.front());
        DBGASSERT(packet->GetOpcode() == pck->GetOpcode());
        DBGASSERT(packet->GetReadableSize() == pck->GetReadableSize());
        DBGASSERT(memcmp(packet->GetReadableBuffer(), pck->GetReadableBuffer(), pck->GetReadableSize()) == 0);
        queue_.pop();
        sDataManager.recv_pck_count_.fetch_add(1);
        return SessionHandleSuccess;
    }
    void SendSomePacket() {
        if (!GetConection()->IsSendBufferEmpty()) {
            if (System::Rand(0, 10000) < 9999)
                return;
        }
        for (int i = 0, total = System::Rand(0, 256); i < total; ++i) {
            const size_t offset = System::Rand(0, sDataManager.packet_string_.size());
            const size_t length = System::Rand(0, sDataManager.packet_string_.size() - offset + 1);
            INetPacket *pck = INetPacket::New(System::Rand(0, 60000), length);
            pck->Append(sDataManager.packet_string_.data() + offset, length);
            PushSendPacket(*pck);
            queue_.push(pck);
            sDataManager.send_pck_count_.fetch_add(1);
        }
        if (sDataManager.send_pck_count_.load() % 100 == 0) {
            //ShutdownSession();
        }
    }
    static void New() {
        ProducerSession *session = new ProducerSession;
        session->SetConnection(sEchoConnectionManager.NewConnection(*session));
        session->GetConection()->AsyncConnect(HOST_STRING, PORT_STRING);
        sEchoSessionManager.AddSession(session);
        sDataManager.producer_list_.insert(session);
    }
private:
    virtual void OnShutdownSession() {
        sDataManager.producer_list_.erase(this);
        if (!IServerMaster::GetInstance().IsEnd())
            New();
    }
    std::queue<INetPacket*> queue_;
};

class EchoSession : public Session {
public:
    EchoSession() : Session(true, true, true) {}
    virtual int HandlePacket(INetPacket *pck) {
        PushSendPacket(*pck);
        sDataManager.trans_pck_count_.fetch_add(1);
        return SessionHandleSuccess;
    }
};

class EchoListener : public Listener, public Singleton<EchoListener> {
public:
    virtual ConnectionManager *GetConnectionManager() { return &sEchoConnectionManager; }
    virtual SessionManager *GetSessionManager() { return &sEchoSessionManager; }
    virtual std::string GetBindAddress() { return HOST_STRING; }
    virtual std::string GetBindPort() { return PORT_STRING; }
    virtual Session *NewSessionObject() { return new EchoSession(); }
};

class EchoServer : public Thread, public Singleton<EchoServer> {
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
        sEchoSessionManager.Update();
        printf("[%d] --- %d <---> %d\r", sDataManager.trans_pck_count_.load(),
            sDataManager.send_pck_count_.load(), sDataManager.recv_pck_count_.load());
        System::Update();
        OS::SleepMS(1);
    }
};

class EchoServerMaster : public IServerMaster, public Singleton<EchoServerMaster> {
public:
    virtual bool InitSingleton() {
        DataManager::newInstance();
        EchoServer::newInstance();
        EchoListener::newInstance();
        EchoConnectionManager::newInstance();
        EchoSessionManager::newInstance();
        return true;
    }
    virtual void FinishSingleton() {
        EchoSessionManager::deleteInstance();
        EchoConnectionManager::deleteInstance();
        EchoListener::deleteInstance();
        EchoServer::deleteInstance();
        DataManager::deleteInstance();
    }
protected:
    virtual bool InitDBPool() { return true; }
    virtual bool LoadDBData() { return true; }
    virtual bool StartServices() {
        sEchoConnectionManager.SetWorkerCount(3);
        sEchoConnectionManager.Start();
        sEchoListener.Start();
        sEchoServer.Start();
        return true;
    }
    virtual void StopServices() {
        sEchoSessionManager.ShutdownAll();
        sEchoConnectionManager.Stop();
        sEchoListener.Stop();
        sEchoServer.Stop();
    }
    virtual void Tick() {}
    virtual std::string GetConfigFile() { return "config"; }
};

void EchoMain(int argc, char **argv)
{
    EchoServerMaster::newInstance();
    sEchoServerMaster.InitSingleton();
    sEchoServerMaster.Initialize(argc, argv);
    sEchoServerMaster.Run(argc, argv);
    sEchoServerMaster.FinishSingleton();
    EchoServerMaster::deleteInstance();
}
