#include "HttpMgr.h"
#include <limits.h>

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
#define write_pipe write
#define read_pipe read
#define close_pipe close
#endif

HttpMgr::HttpMgr()
{
    pipefd_[1] = pipefd_[0] = INVALID_SOCKET;
}

HttpMgr::~HttpMgr()
{
}

bool HttpMgr::Initialize()
{
    if (pipe(pipefd_) != 0)
        return false;
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
    if (pipefd_[0] != INVALID_SOCKET)
        close_pipe(pipefd_[0]);
    if (pipefd_[1] != INVALID_SOCKET)
        close_pipe(pipefd_[1]);
    pipefd_[1] = pipefd_[0] = INVALID_SOCKET;
}

HttpClient *HttpMgr::AppendTask(Observer *observer, const char *url, const char *filepath)
{
    HttpClient *client = new HttpClient;
    client->SetResource(url, nullptr, nullptr);
    client->SetStorage(filepath);
    AppendTask(observer, client);
    return client;
}

void HttpMgr::AppendTask(Observer *observer, HttpClient *client)
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
    std::pair<HttpClient*, Observer*> wrpair;
    while (waiting_room_.Dequeue(wrpair)) {
        client_list_.insert(wrpair.first);
        if (wrpair.second != nullptr)
            observer_list_.insert(wrpair);
    }

    std::pair<HttpClient*, bool> rbpair;
    while (recycle_bin_.Dequeue(rbpair)) {
        client_list_.erase(rbpair.first);
        observer_list_.erase(rbpair.first);
        if (rbpair.second)
            delete rbpair.first;
    }
}

void HttpMgr::UpdateClients()
{
    std::vector<struct pollfd> pollfd_list;
    pollfd_list.reserve(client_list_.size() + 1);
    std::vector<HttpClient*> subject_list;
    subject_list.reserve(client_list_.size());
    struct pollfd spollfd;
    for (auto client : client_list_) {
        if (client->RegisterObserver(spollfd)) {
            pollfd_list.push_back(spollfd);
            subject_list.push_back(client);
        }
    }

    spollfd.fd = pipefd_[0];
    spollfd.events = POLLRDNORM;
    pollfd_list.push_back(spollfd);

    int timeout = subject_list.empty() ? INT_MAX : 1000;
    int ret = poll(pollfd_list.data(), pollfd_list.size(), timeout);
    if (ret == SOCKET_ERROR)
        return;

    if (ret > 0) {
        char a[64];
        struct pollfd &pipefd = pollfd_list.back();
        if ((pipefd.revents & POLLRDNORM) != 0)
            read_pipe(pipefd.fd, a, sizeof(a));
    }

    const size_t subject_number = subject_list.size();
    for (size_t index = 0; index < subject_number; ++index) {
        HttpClient *client = subject_list[index];
        client->CheckTimeout();
        if (ret > 0) {
            struct pollfd &sockfd = pollfd_list[index];
            if ((sockfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
                client->HandleExceptable();
            if ((sockfd.revents & POLLRDNORM) != 0)
                client->HandleReadable();
            if ((sockfd.revents & POLLWRNORM) != 0)
                client->HandleWritable();
        }
        if (client->error() != HttpClient::ErrorNone) {
            auto iterator = observer_list_.find(client);
            if (iterator != observer_list_.end())
                iterator->second->UpdateSubject(client, client->error());
        }
    }
}
