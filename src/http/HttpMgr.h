#pragma once

#include "Singleton.h"
#include "Thread.h"
#include <unordered_map>
#include <unordered_set>
#include "HttpClient.h"
#include "MultiBufferQueue.h"

class HttpMgr : public Thread, public Singleton<HttpMgr>
{
public:
    THREAD_RUNTIME(HttpMgr)

    class ITaskObserver {
    public:
        virtual void UpdateTaskStatus(void *task, int error) = 0;
    };

    HttpMgr();
    virtual ~HttpMgr();

    HttpClient *AppendTask(ITaskObserver *observer, const char *filepath,
        const char *url, const char *referer = nullptr, const char *cookie = nullptr);

    void AppendTask(ITaskObserver *observer, HttpClient *client);
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
    CURLM *multi_handle_;

    std::unordered_set<HttpClient*> client_list_;
    std::unordered_map<HttpClient*, ITaskObserver*> observer_list_;
    MultiBufferQueue<std::pair<HttpClient*, ITaskObserver*>, 128> waiting_room_;
    MultiBufferQueue<std::pair<HttpClient*, bool>, 128> recycle_bin_;
};

#define sHttpMgr (*HttpMgr::instance())
