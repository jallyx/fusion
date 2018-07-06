#include "HttpMgr.h"
#include "Macro.h"

#if defined(_WIN32)
int pipe(int pipefd[2])
{
    struct sockaddr_in addr;
    int namelen = sizeof(addr);
    SOCKET sock = INVALID_SOCKET;
    pipefd[1] = pipefd[0] = INVALID_SOCKET;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        goto clean;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
        goto clean;
    if (listen(sock, 1) != 0)
        goto clean;
    if (getsockname(sock, (struct sockaddr *)&addr, &namelen) != 0)
        goto clean;
    if ((pipefd[0] = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        goto clean;
    if (connect(pipefd[0], (struct sockaddr *)&addr, namelen) != 0)
        goto clean;
    if ((pipefd[1] = accept(sock, nullptr, nullptr)) == INVALID_SOCKET)
        goto clean;
    closesocket(sock);
    return 0;
clean:
    if (sock != INVALID_SOCKET)
        closesocket(sock);
    if (pipefd[0] != INVALID_SOCKET)
        closesocket(pipefd[0]);
    if (pipefd[1] != INVALID_SOCKET)
        closesocket(pipefd[1]);
    pipefd[1] = pipefd[0] = INVALID_SOCKET;
    return -1;
}
#endif

#if defined(_WIN32)
#define write_pipe(a,b,c) send(a,b,c,0);
#define read_pipe(a,b,c) recv(a,b,c,0);
#define close_pipe closesocket
#else
#include <unistd.h>
#define write_pipe write
#define read_pipe read
#define close_pipe close
#endif

HttpMgr::HttpMgr()
: multi_handle_(nullptr)
{
    pipefd_[1] = pipefd_[0] = INVALID_SOCKET;
}

HttpMgr::~HttpMgr()
{
}

bool HttpMgr::Initialize()
{
    if ((multi_handle_ = curl_multi_init()) == nullptr) {
        return false;
    }
    if (pipe(pipefd_) != 0) {
        return false;
    }
    return true;
}

void HttpMgr::Kernel()
{
    CheckClients();
    UpdateClients();
}

void HttpMgr::Abort()
{
    WakeWorker();
}

void HttpMgr::Finish()
{
    if (multi_handle_ != nullptr) {
        curl_multi_cleanup(multi_handle_);
        multi_handle_ = nullptr;
    }

    if (pipefd_[0] != INVALID_SOCKET) {
        close_pipe(pipefd_[0]);
        pipefd_[0] = INVALID_SOCKET;
    }
    if (pipefd_[1] != INVALID_SOCKET) {
        close_pipe(pipefd_[1]);
        pipefd_[1] = INVALID_SOCKET;
    }
}

HttpClient *HttpMgr::AppendTask(ITaskObserver *observer, const char *filepath,
        const char *url, const char *referer, const char *cookie)
{
    HttpClient *client = new HttpClient;
    client->SetResource(url, referer, cookie);
    client->SetStorage(filepath);
    AppendTask(observer, client);
    return client;
}

void HttpMgr::AppendTask(ITaskObserver *observer, HttpClient *client)
{
    waiting_room_.Enqueue(std::make_pair(client, observer));
    WakeWorker();
}

void HttpMgr::RemoveTask(HttpClient *client, bool deletable)
{
    recycle_bin_.Enqueue(std::make_pair(client, deletable));
    WakeWorker();
}

void HttpMgr::WakeWorker()
{
    write_pipe(pipefd_[1], "a", 1);
}

void HttpMgr::CheckClients()
{
    std::pair<HttpClient*, ITaskObserver*> wrpair;
    while (waiting_room_.Dequeue(wrpair)) {
        wrpair.first->Register(multi_handle_);
        client_list_.insert(wrpair.first);
        if (wrpair.second != nullptr) {
            observer_list_.insert(wrpair);
        }
    }

    std::pair<HttpClient*, bool> rbpair;
    while (recycle_bin_.Dequeue(rbpair)) {
        rbpair.first->Unregister(multi_handle_);
        client_list_.erase(rbpair.first);
        observer_list_.erase(rbpair.first);
        if (rbpair.second) {
            delete rbpair.first;
        }
    }
}

void HttpMgr::UpdateClients()
{
    int running_handles = 0;
    if (curl_multi_perform(multi_handle_, &running_handles) != CURLM_OK) {
        return;
    }

    curl_waitfd pipefd;
    pipefd.fd = pipefd_[0];
    pipefd.events = CURL_WAIT_POLLIN;
    int numfds = 0;
    if (curl_multi_wait(multi_handle_, &pipefd, 1, 1000, &numfds) != CURLM_OK) {
        return;
    }

    if (numfds > 0) {
        char a[64];
        if ((pipefd.revents & CURL_WAIT_POLLIN) != 0) {
            read_pipe(pipefd.fd, a, sizeof(a));
        }
    }

    while (true) {
        int msgs_in_queue = 0;
        CURLMsg *m = curl_multi_info_read(multi_handle_, &msgs_in_queue);
        if (m != nullptr) {
            if (m->msg == CURLMSG_DONE) {
                HttpClient *client = nullptr;
                CURLcode ret = curl_easy_getinfo(m->easy_handle, CURLINFO_PRIVATE, &client);
                if (ret == CURLE_OK && client != nullptr) {
                    client->Done(m->data.result);
                    auto itr = observer_list_.find(client);
                    if (itr != observer_list_.end()) {
                        itr->second->UpdateTaskStatus(client, client->error());
                    }
                }
            }
        } else {
            break;
        }
    }
}
