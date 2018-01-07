#pragma once

#include "Singleton.h"
#include "Thread.h"
#include <unordered_map>
#include <unordered_set>
#include "HttpClient.h"
#include "ThreadSafeQueue.h"

class HttpMgr : public Thread, public Singleton<HttpMgr>
{
public:
    THREAD_RUNTIME(HttpMgr)

    class Observer {
    public:
        virtual void UpdateSubject(void *task, int error) = 0;
    };

    HttpMgr();
    virtual ~HttpMgr();

    HttpClient *AppendTask(Observer *observer, const char *url, const char *filepath);
    void AppendTask(Observer *observer, HttpClient *client);
    void RemoveTask(HttpClient *client, bool deletable);
    void WakeWorker();

private:
    virtual bool Initialize();
    virtual void Kernel();
    virtual void Abort();
    virtual void Finish();

    void CheckClients();
    void UpdateClients();

    int pipefd_[2];
    std::unordered_set<HttpClient*> client_list_;
    std::unordered_map<HttpClient*, Observer*> observer_list_;
    ThreadSafeQueue<std::pair<HttpClient*, Observer*>> waiting_room_;
    ThreadSafeQueue<std::pair<HttpClient*, bool>> recycle_bin_;
};

#define sHttpMgr (*HttpMgr::instance())
