#include "network/ConnectionManager.h"
#include "network/SessionManager.h"
#include "network/Listener.h"
#include "ServerMaster.h"
#include "Debugger.h"
#include "System.h"
#include "OS.h"

#define HOST_STRING "127.0.0.1"
#define PORT_STRING "9999"
#define sParallelServer (*ParallelServer::instance())
#define sParallelListener (*ParallelListener::instance())
#define sParallelServerMaster (*ParallelServerMaster::instance())
#define sParallelConnectionManager (*ParallelConnectionManager::instance())
#define sParallelSessionManager (*ParallelSessionManager::instance())
#define sParallelWorkerManager (*ParallelWorkerManager::instance())
#define sDataManager (*DataManager::instance())
#define sProducerSession (*ProducerSession::instance())

class ParallelConnectionManager : public ConnectionManager, public Singleton<ParallelConnectionManager> {
};
class ParallelSessionManager : public SessionManager, public Singleton<ParallelSessionManager> {
};

class DataManager : public Singleton<DataManager> {
public:
    DataManager() {
        packet_string_.resize(65536 * 5);
        for (size_t i = 0, total = packet_string_.size(); i < total; ++i) {
            packet_string_[i] = System::Rand(0, 256);
        }
    }
    virtual ~DataManager() {
        for (INetPacket *pck : pck_list_) {
            delete pck;
        }
    }
    void HandlePacket(INetPacket &pck) {
        for (size_t i = 0;;++i) {
            INetPacket *packet = GetPacket(i);
            if (packet != nullptr) {
                if (packet->GetOpcode() == pck.GetOpcode() &&
                    packet->GetReadableSize() == pck.GetReadableSize() &&
                    memcmp(packet->GetReadableBuffer(), pck.GetReadableBuffer(), pck.GetReadableSize()) == 0)
                {
                    PopPacket(i);
                    delete packet;
                    break;
                }
            } else {
                DBGASSERT(packet != nullptr);
                break;
            }
        }
    }
    INetPacket *GetPacket(size_t i) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return i < pck_list_.size() ? pck_list_[i] : nullptr;
    }
    void PopPacket(size_t i) {
        std::lock_guard<std::mutex> lock(mutex_);
        pck_list_.erase(pck_list_.begin() + i);
    }
    void PushPacket(INetPacket *pck) {
        std::lock_guard<std::mutex> lock(mutex_);
        pck_list_.push_back(pck);
    }
    std::string packet_string_;
    std::atomic<int> send_pck_count_{0}, recv_pck_count_{0};
    std::vector<INetPacket*> pck_list_;
    mutable std::mutex mutex_;
};

class ProducerSession : public Session, public Singleton<ProducerSession> {
public:
    ProducerSession() : Session(true, true, true) {}
    virtual int HandlePacket(INetPacket *pck) {
        return SessionHandleSuccess;
    }
    void SendSomePacket() {
        if (GetSendDataSize() > 65536 * 256) {
            return;
        }
        for (int i = 0, total = System::Rand(0, 256); i < total; ++i) {
            const size_t offset = System::Rand(0, sDataManager.packet_string_.size());
            const size_t length = System::Rand(0, sDataManager.packet_string_.size() - offset + 1);
            INetPacket *pck = INetPacket::New(System::Rand(0, 60000), length);
            pck->Append(sDataManager.packet_string_.data() + offset, length);
            sDataManager.PushPacket(pck);
            PushSendPacket(*pck);
            sDataManager.send_pck_count_.fetch_add(1);
        }
    }
};

class ConsumerSession : public Session {
public:
    ConsumerSession() : Session(true, true, true) {}
    virtual int HandlePacket(INetPacket *pck) {
        sDataManager.HandlePacket(*pck);
        sDataManager.recv_pck_count_.fetch_add(1);
        return SessionHandleSuccess;
    }
};

class ParallelWorkerManager : public ThreadPool, public Singleton<ParallelWorkerManager> {
public:
    class ParallelWorker : public Thread {
        void Kernel() {
            if (sProducerSession.IsStatus(Session::Running)) {
                sProducerSession.SendSomePacket();
            }
            OS::SleepMS(1);
        }
    };
    virtual bool Prepare() {
        for (int i = 0; i < 6; ++i) {
            PushThread(new ParallelWorker());
        }
        return true;
    }
};

class ParallelListener : public Listener, public Singleton<ParallelListener> {
public:
    virtual ConnectionManager *GetConnectionManager() { return &sParallelConnectionManager; }
    virtual SessionManager *GetSessionManager() { return &sParallelSessionManager; }
    virtual std::string GetBindAddress() { return HOST_STRING; }
    virtual std::string GetBindPort() { return PORT_STRING; }
    virtual Session *NewSessionObject() { return new ConsumerSession(); }
};

class ParallelServer : public Thread, public Singleton<ParallelServer> {
public:
    virtual bool Initialize() {
        sProducerSession.SetConnection(sParallelConnectionManager.NewConnection(sProducerSession));
        sProducerSession.GetConnection()->AsyncConnect(HOST_STRING, PORT_STRING);
        sParallelSessionManager.AddSession(&sProducerSession);
        return true;
    }
    virtual void Kernel() {
        sParallelSessionManager.Update();
        printf("%d --> %d\r", sDataManager.send_pck_count_.load(),
            sDataManager.recv_pck_count_.load());
        System::Update();
        OS::SleepMS(1);
    }
};

class ParallelServerMaster : public IServerMaster, public Singleton<ParallelServerMaster> {
public:
    virtual bool InitSingleton() {
        ProducerSession::newInstance();
        DataManager::newInstance();
        ParallelServer::newInstance();
        ParallelListener::newInstance();
        ParallelConnectionManager::newInstance();
        ParallelSessionManager::newInstance();
        ParallelWorkerManager::newInstance();
        return true;
    }
    virtual void FinishSingleton() {
        ParallelWorkerManager::deleteInstance();
        ParallelSessionManager::deleteInstance();
        ParallelConnectionManager::deleteInstance();
        ParallelListener::deleteInstance();
        ParallelServer::deleteInstance();
        DataManager::deleteInstance();
        ProducerSession::deleteInstance();
    }
protected:
    virtual bool InitDBPool() { return true; }
    virtual bool LoadDBData() { return true; }
    virtual bool StartServices() {
        sParallelConnectionManager.SetWorkerCount(3);
        sParallelConnectionManager.Start();
        sParallelWorkerManager.Start();
        sParallelListener.Start();
        sParallelServer.Start();
        return true;
    }
    virtual void StopServices() {
        sParallelWorkerManager.Stop();
        sParallelSessionManager.Stop();
        sParallelConnectionManager.Stop();
        sParallelListener.Stop();
        sParallelServer.Stop();
    }
    virtual void Tick() {}
    virtual std::string GetConfigFile() { return "config"; }
};

void ParallelMain(int argc, char **argv)
{
    ParallelServerMaster::newInstance();
    sParallelServerMaster.InitSingleton();
    sParallelServerMaster.Initialize(argc, argv);
    sParallelServerMaster.Run(argc, argv);
    sParallelServerMaster.FinishSingleton();
    ParallelServerMaster::deleteInstance();
}
