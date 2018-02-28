#include "HttpClient.h"
#include <assert.h>
#include <string.h>
#include <time.h>
#include <algorithm>
#include <memory>
#include "OS.h"

#if defined(_WIN32)
    #include <WS2tcpip.h>
#else
    #include <netdb.h>
#endif

#define USER_AGENT "9FANG MMORPG"
#define ACTIVE_TIMEOUT_MAX (10)

static char dummy_head[2] =
{
    0x8 + 0x7 * 0x10,
    (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
};

HttpClient::HttpClient()
: filesize_(-1)
, filepos_(0)
, is_requested_(false)
, is_replyed_(false)
, is_chunked_(false)
, is_chunking_(false)
, chunk_size_(1)
, chunk_pos_(0)
, compress_mode_(HTTP_COMPRESS_IDENTITY)
, is_dummy_stream_initialised_(false)
, is_stream_initialized_(false)
, is_stream_ended_(false)
, sock_(INVALID_SOCKET)
, error_(ErrorNone)
, forward_error_(ErrorNone)
, is_keep_alive_(false)
, is_redirected_(false)
, user_data_(nullptr)
, active_timeout_(ACTIVE_TIMEOUT_MAX)
, active_stamp_(0)
{
}

HttpClient::~HttpClient()
{
    if (is_stream_initialized_)
        deflateEnd(&data_stream_);
    if (sock_ != INVALID_SOCKET)
        closesocket(sock_);
}

void HttpClient::SetResource(const char *url, const char *referer, const char *cookie)
{
    assert(url != nullptr && *url != '\0');
    url_.assign(url);
    if (referer && *referer)
        referer_.assign(referer);
    if (cookie && *cookie)
        cookie_.assign(cookie);
}

void HttpClient::SetStorage(const char *filepath)
{
    if (filepath != nullptr && *filepath != '\0') {
        OS::CreateFilePath(filepath);
        filepath_.assign(filepath);
        ofstream_.open(filepath, std::ios::app|std::ios::ate|std::ios::binary);
        if (ofstream_.is_open())
            filepos_ = ofstream_.tellp();
    } else {
        filedata_.append(1, '\0');
        osstream_.str(std::string());
    }
}

void HttpClient::SetListener(const std::function<void(const void *, size_t)> &listener)
{
    assert(listener && "listener must is callable.");
    listener_ = listener;
}

bool HttpClient::RegisterObserver(struct pollfd &sockfd)
{
    if (url_.empty())
        return false;

    if (error_ != ErrorNone)
        return false;

    if (sock_ == INVALID_SOCKET) {
        if (!Connect()) {
            error_ = ConnectFailed;
            return false;
        }
    }

    sockfd.fd = sock_;
    sockfd.events = is_requested_ ? POLLRDNORM : POLLWRNORM;

    if (active_stamp_ == 0)
        time(&active_stamp_);

    return true;
}

void HttpClient::HandleExceptable()
{
    if (error_ != ErrorNone)
        return;

    time(&active_stamp_);

    if (!is_requested_ && request_.empty())
        error_ = ConnectFailed;
    else
        error_ = LinkInterrupt;
}

void HttpClient::HandleReadable()
{
    if (error_ != ErrorNone)
        return;

    time(&active_stamp_);

    while (!is_replyed_) {
        char buffer[1024];
        int length = recv(sock_, buffer, sizeof(buffer), 0);
        if (length == SOCKET_ERROR) {
            if (GET_SOCKET_ERROR() != ERROR_WOULDBLOCK)
                error_ = LinkInterrupt;
            break;
        }
        if (length == 0) {
            error_ = LinkInterrupt;
            break;
        }
        reply_.append(buffer, length);
        ParseHeader();
        switch (error_) {
        case ErrorNone:
            if (!is_replyed_)
                break;
            if (!reply_.empty())
                ParseBody();
            if (!IsFinishTask(false))
                break;
        case TaskFinished:
            FinishTask();
        default:
            return;
        }
    }

    while (is_replyed_) {
        char buffer[1024];
        int length = recv(sock_, buffer, sizeof(buffer), 0);
        if (length == SOCKET_ERROR) {
            if (GET_SOCKET_ERROR() != ERROR_WOULDBLOCK)
                error_ = LinkInterrupt;
            break;
        }
        if (length != 0) {
            reply_.append(buffer, length);
            ParseBody();
        }
        if (IsFinishTask(length == 0)) {
            FinishTask();
            break;
        }
        if (length == 0) {
            error_ = LinkInterrupt;
            break;
        }
    }
}

void HttpClient::HandleWritable()
{
    if (error_ != ErrorNone)
        return;

    if (is_requested_)
        return;

    time(&active_stamp_);

    if (request_.empty()) {
        if (!BuildHeader()) {
            error_ = RequestInvalid;
            return;
        }
    }
    while (true) {
        int length = send(sock_, request_.data(), request_.size(), 0);
        if (length == SOCKET_ERROR) {
            if (GET_SOCKET_ERROR() != ERROR_WOULDBLOCK)
                error_ = LinkInterrupt;
            break;
        }
        request_.erase(0, length);
        if (request_.empty()) {
            ProvideBody();
            if (error_ != ErrorNone)
                break;
            if (request_.empty()) {
                is_requested_ = true;
                break;
            }
        }
    }
}

void HttpClient::CheckTimeout()
{
    const time_t curtime = time(nullptr);
    if (active_stamp_ + active_timeout_ <= curtime) {
        if (!is_requested_ && request_.empty())
            error_ = ConnectFailed;
        else
            error_ = LinkZombie;
        return;
    }
    if (active_stamp_ > curtime) {
        active_stamp_ = curtime;
    }
}

void HttpClient::ResetStatus()
{
    if (is_stream_initialized_)
        deflateEnd(&data_stream_);
    if (ofstream_.is_open())
        ofstream_.close();
    osstream_.str(std::string());

    url_.clear();
    referer_.clear();
    cookie_.clear();
    location_.clear();
    filepath_.clear();
    ofstream_.clear();
    filedata_.clear();
    osstream_.clear();
    request_.clear();
    reply_.clear();

    listener_ = nullptr;
    filesize_ = -1;
    filepos_ = 0;
    is_requested_ = false;
    is_replyed_ = false;
    is_chunked_ = false;
    is_chunking_ = false;
    chunk_size_ = 1;
    chunk_pos_ = 0;
    compress_mode_ = HTTP_COMPRESS_IDENTITY;
    is_dummy_stream_initialised_ = false;
    is_stream_initialized_ = false;
    is_stream_ended_ = false;
    error_ = ErrorNone;
    forward_error_ = ErrorNone;
    is_redirected_ = false;
    active_stamp_ = 0;
}

void HttpClient::Reset()
{
    ResetStatus();
    is_keep_alive_ = false;
    if (sock_ != INVALID_SOCKET) {
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }
}

bool HttpClient::Connect()
{
    const char *hostptr = strstr(url_.c_str(), "://");
    if (hostptr != nullptr) {
        hostptr += 3;
    } else {
        return false;
    }

    const char *portptr = strchr(hostptr, ':');
    const char *pathptr = strchr(hostptr, '/');
    if (portptr != nullptr && pathptr != nullptr && portptr > pathptr)
        portptr = nullptr;

    std::string host;
    if (portptr != nullptr)
        host.assign(hostptr, portptr);
    else if (pathptr != nullptr)
        host.assign(hostptr, pathptr);
    else
        host.assign(hostptr);

    std::string port;
    if (portptr != nullptr) {
        if (pathptr != nullptr)
            port.assign(portptr + 1, pathptr);
        else
            port.assign(portptr + 1);
    } else {
        port.assign("80");
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = 0;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0)
        return false;

    std::unique_ptr<addrinfo, decltype(freeaddrinfo)*> _(res, freeaddrinfo);
    sock_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_ == INVALID_SOCKET)
        return false;

    if (!OS::non_blocking(sock_))
        return false;

    if (connect(sock_, res->ai_addr, res->ai_addrlen) != 0) {
        if (GET_SOCKET_ERROR() != ERROR_INPROGRESS)
            return false;
    }

    return true;
}

bool HttpClient::BuildHeader()
{
    const char *hostptr = strstr(url_.c_str(), "://");
    if (hostptr != nullptr) {
        hostptr += 3;
    } else {
        return false;
    }

    const char *pathptr = strchr(hostptr, '/');

    std::string host;
    if (pathptr != nullptr)
        host.assign(hostptr, pathptr);
    else
        host.assign(hostptr);

    const char *path = pathptr != nullptr ? pathptr : "/";
    request_ = is_redirected_ ?
        HttpClient::BuildHeader(path, host, referer_, cookie_) :
        BuildHeader(path, host, referer_, cookie_);

    return true;
}

void HttpClient::ProvideBody()
{
    is_redirected_ ? HttpClient::ProvideBody(request_) : ProvideBody(request_);
}

void HttpClient::StartRedirect()
{
    bool is_keep_alive = false;
    const char *hostptr = strstr(location_.c_str(), "://");
    if (hostptr != nullptr) {
        const char *pathptr = strchr(hostptr + 3, '/');
        const size_t length = pathptr != nullptr ?
            pathptr - location_.c_str() : location_.size();
        is_keep_alive =
            strncasecmp(location_.c_str(), url_.c_str(), length) == 0 &&
            (url_.size() == length || url_[length] == '/');
    }

    std::string url = location_;
    std::string referer = referer_;
    std::string cookie = cookie_;
    std::string filepath = filepath_;
    std::string filedata = filedata_;
    std::function<void(const void *, size_t)> listener = listener_;
    if (is_keep_alive && is_keep_alive_) {
        ResetStatus();
    } else {
        Reset();
    }

    SetResource(url.c_str(), referer.c_str(), cookie.c_str());
    if (listener) {
        SetListener(listener);
    } else if (!filepath.empty() || !filedata.empty()) {
        SetStorage(filepath.c_str());
    }
    time(&active_stamp_);
    is_redirected_ = true;
}

std::string HttpClient::BuildHeader(
    const char *path, const std::string &host,
    const std::string &referer, const std::string &cookie)
{
    std::ostringstream stream;
    stream << "GET " << path << " HTTP/1.1\r\n";
    stream << "HOST: " << host << "\r\n";
    stream << "User-Agent: " USER_AGENT "\r\n";
    stream << "Accept: text/html,*/*;q=0.8\r\n";
    if (filepos() == 0)
        stream << "Accept-Encoding: gzip, deflate\r\n";
    if (is_keep_alive())
        stream << "Connection: keep-alive\r\n";
    else
        stream << "Connection: close\r\n";
    if (!referer.empty())
        stream << "Referer: " << referer << "\r\n";
    if (!cookie.empty())
        stream << "Cookie: " << cookie << "\r\n";
    if (filepos() != 0)
        stream << "Range: bytes=" << filepos() << "-\r\n";
    stream << "\r\n";
    return stream.str();
}

void HttpClient::ScanHeader(const char *reply)
{
    const char *content_length = "\nContent-Length:";
    const char *content_encoding = "\nContent-Encoding:";
    const char *transfer_encoding = "\nTransfer-Encoding:";
    const char *ptr = nullptr;
    if ((ptr = strcasestr(reply, content_length)) != nullptr) {
        sscanf(ptr + strlen(content_length), "%lld", &filesize_);
    }
    if ((ptr = strcasestr(reply, content_encoding)) != nullptr) {
        char type[10 + 1] = "";  // deflate,gzip,x-gzip,compress,x-compress
        sscanf(ptr + strlen(content_encoding), "%10s", type);
        if (strcasecmp(type, "deflate") == 0)
            compress_mode_ = HTTP_COMPRESS_DEFLATE;
        else if (strcasecmp(type, "gzip") == 0 || strcasecmp(type, "x-gzip") == 0)
            compress_mode_ = HTTP_COMPRESS_GZIP;
        else if (strcasecmp(type, "compress") == 0 || strcasecmp(type, "x-compress") == 0)
            compress_mode_ = HTTP_COMPRESS_COMPRESS;
    }
    if ((ptr = strcasestr(reply, transfer_encoding)) != nullptr) {
        char type[7 + 1] = "";  // chunked
        sscanf(ptr + strlen(transfer_encoding), "%7s", type);
        is_chunked_ = strcasecmp(type, "chunked") == 0;
    }
}

void HttpClient::ParseHeader()
{
    const char *reply = reply_.c_str();
    const char *bodyptr = strstr(reply, "\r\n\r\n");
    if (bodyptr != nullptr) {
        bodyptr += 4;
    } else {
        bodyptr = strstr(reply, "\n\n");
        if (bodyptr != nullptr)
            bodyptr += 2;
        else
            return;
    }

    is_replyed_ = true;

    size_t length = bodyptr - reply;
    reply_[length - 1] = '\0';

    uint64 filepos = filepos_;
    filepos_ = 0;
    ScanHeader(reply);

    int response = -1;
    const char *http_version = "HTTP/";  // e.g. HTTP/0.9 HTTP/1.1
    if (strncasecmp(reply, http_version, strlen(http_version)) == 0)
        sscanf(reply, "%*s %d", &response);

    switch (response) {
    case 200: {
        if (filepos != 0) {
            DiscardData();
        }
        break;
    }
    case 206: {
        const char *content_range = "\nContent-Range:";
        const char *ptr = nullptr;
        if ((ptr = strcasestr(reply, content_range)) != nullptr) {
            sscanf(ptr + strlen(content_range),
                "%*s%llu%*[^/]/%lld", &filepos_, &filesize_);
            if (filepos_ != filepos)
                error_ = ReplyInvalid;
        } else {
            error_ = ReplyInvalid;
        }
        break;
    }
    case 301:
    case 302: {
        const char *location = "\nLocation:";
        const char *ptr = nullptr;
        if ((ptr = strcasestr(reply, location)) != nullptr) {
            const char *begptr = ptr + strlen(location);
            const char *endptr = strchr(begptr, '\n');
            location_.assign(begptr, endptr);
            location_.erase(location_.find_last_not_of("\x20\t\r") + 1);
            location_.erase(0, location_.find_first_not_of("\x20\t"));
            forward_error_ = TaskFinished;
        } else {
            error_ = ReplyInvalid;
        }
        break;
    }
    case 416: {
        const char *content_range = "\nContent-Range:";
        const char *ptr = nullptr;
        if ((ptr = strcasestr(reply, content_range)) != nullptr) {
            uint64 filesize = 0;
            sscanf(ptr + strlen(content_range), "%*[^/]/%llu", &filesize);
            if (filesize != filepos)
                ptr = nullptr;
        }
        if (ptr == nullptr) {
            DiscardData();
            location_ = url_;
        }
        forward_error_ = TaskFinished;
        break;
    }
    default:
        error_ = NotImplement;
        break;
    }

    reply_.erase(0, length);
}

void HttpClient::ParseBody()
{
    const char * const dataptr = reply_.data();
    const char *begptr = dataptr;
    if (is_chunked_) {
        const char * const endptr = dataptr + reply_.size();
        while (begptr < endptr) {
            if (!is_chunking_) {
                const char *ptr = (const char *)memchr(begptr, '\n', endptr - begptr);
                if (ptr == nullptr) break;
                chunk_size_ = chunk_pos_ = 0;
                sscanf(begptr, "%x", &chunk_size_);
                begptr = ptr + 1;
                is_chunking_ = true;
            }
            if (chunk_size_ > chunk_pos_ && endptr > begptr) {
                uint32 length = std::min(chunk_size_ - chunk_pos_, uint32(endptr - begptr));
                if (TryDigestData(begptr, length)) {
                    chunk_pos_ += length;
                    begptr += length;
                } else {
                    break;
                }
            }
            if (chunk_size_ <= chunk_pos_ && endptr > begptr) {
                const char *ptr = (const char *)memchr(begptr, '\n', endptr - begptr);
                if (ptr == nullptr) break;
                if (chunk_size_ == 0) {
                    do {
                        begptr = ptr + 1;
                        if (ptr == dataptr) break;
                        if (ptr - 1 == dataptr && ptr[-1] == '\r') break;
                        if (ptr - 1 >= dataptr && ptr[-1] == '\n') break;
                        if (ptr - 3 >= dataptr && memcmp(ptr - 3, "\r\n\r", 3) == 0) break;
                        ptr = (const char *)memchr(begptr, '\n', endptr - begptr);
                    } while (ptr != nullptr);
                    if (ptr == nullptr)
                        break;
                }
                begptr = ptr + 1;
                is_chunking_ = false;
                if (chunk_size_ == 0)
                    break;
            }
        }
    } else {
        uint32 length = filesize_ != -1 ?
            std::min(size_t(filesize_ - filepos_), reply_.size()) : reply_.size();
        if (TryDigestData(begptr, length)) {
            begptr += length;
        }
    }
    reply_.erase(0, begptr - dataptr);
}

bool HttpClient::TryDigestData(const void *data, size_t size)
{
    filepos_ += size;
    if (error_ != ErrorNone || forward_error_ != ErrorNone)
        return true;
    if (!listener_ && filepath_.empty() && filedata_.empty())
        return true;
    if (InflateData(data, size))
        return true;
    filepos_ -= size;
    return false;
}

bool HttpClient::InflateData(const void *data, size_t size)
{
    if (is_stream_ended_)
        return true;

    switch (compress_mode_) {
    case HTTP_COMPRESS_GZIP:
    case HTTP_COMPRESS_DEFLATE: {
        if (!is_stream_initialized_) {
            is_stream_initialized_ = true;
            memset(&data_stream_, 0, sizeof(data_stream_));
            inflateInit2(&data_stream_, 47);
        }
mark:   size_t offset = 0;
        if (!is_dummy_stream_initialised_)
            offset = data_stream_.total_in;
        data_stream_.next_in = (Bytef*)data + offset;
        data_stream_.avail_in = (uInt)size - offset;
        do {
            char buffer[1024];
            data_stream_.next_out = (Bytef*)buffer;
            data_stream_.avail_out = sizeof(buffer);
            switch (inflate(&data_stream_, Z_NO_FLUSH)) {
            case Z_STREAM_END: is_stream_ended_ = true;
            case Z_OK: {
                int length = sizeof(buffer) - data_stream_.avail_out;
                if (length > 0) {
                    is_dummy_stream_initialised_ = true;
                    WriteData(buffer, length);
                }
                break;
            }
            case Z_BUF_ERROR:
                break;
            case Z_DATA_ERROR:
                if (!is_dummy_stream_initialised_) {
                    is_dummy_stream_initialised_ = true;
                    inflateReset(&data_stream_);
                    data_stream_.next_in = (Bytef*)dummy_head;
                    data_stream_.avail_in = sizeof(dummy_head);
                    inflate(&data_stream_, Z_NO_FLUSH);
                    goto mark;
                }
            default:
                error_ = DataAbnormal;
                return false;
            }
        } while (!is_stream_ended_ &&
                 (data_stream_.avail_in != 0 || data_stream_.avail_out == 0));
        if (!is_dummy_stream_initialised_ && !is_stream_ended_)
            return false;
        break;
    }
    default:
        WriteData(data, size);
        break;
    }

    return true;
}

void HttpClient::WriteData(const void *data, size_t size)
{
    if (listener_) {
        listener_(data, size);
    } else {
        std::ostream &ostream =
            !filepath_.empty() ?
            static_cast<std::ostream&>(ofstream_) :
            static_cast<std::ostream&>(osstream_);
        ostream.write((const char *)data, size);
    }
}

void HttpClient::DiscardData()
{
    if (!filepath_.empty()) {
        ofstream_.close();
        ofstream_.clear();
        ofstream_.open(filepath_.c_str(), std::ios::trunc|std::ios::binary);
    } else if (!filedata_.empty()) {
        osstream_.str(std::string());
        osstream_.clear();
    }
}

bool HttpClient::IsFinishTask(bool is_connection_closed) const
{
    if (is_chunked_) {
        if (!is_chunking_ && chunk_size_ == 0)
            return true;
        return false;
    }

    if (filesize_ == -1) {
        if (is_connection_closed)
            return true;
        return false;
    }

    if ((uint64)filesize_ <= filepos_)
        return true;
    return false;
}

void HttpClient::FinishTask()
{
    if (!filepath_.empty()) {
        ofstream_.close();
        ofstream_.clear();
    } else if (!filedata_.empty()) {
        filedata_ = osstream_.str();
        osstream_.str(std::string());
        osstream_.clear();
    }

    if (!is_keep_alive_) {
        if (sock_ != INVALID_SOCKET) {
            closesocket(sock_);
            sock_ = INVALID_SOCKET;
        }
    }

    if (location_.empty()) {
        error_ = TaskFinished;
        if (forward_error_ != ErrorNone) {
            error_ = forward_error_;
        }
    } else {
        StartRedirect();
    }
}

std::string HttpClient::EncodeUrl(const std::string& data)
{
    std::string result;
    result.reserve(data.length() * 3);
    for (unsigned char value : data) {
        if (value == '\x20') {
            result.append(1, '+');
        } else if (isalnum(value) || strchr("-_.*", value)) {
            result.append(1, char(value));
        } else {
            char buffer[3+1];
            sprintf(buffer, "%%%02x", unsigned(value));
            result.append(buffer, 3);
        }
    }
    return result;
}
