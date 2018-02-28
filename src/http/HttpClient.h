#pragma once

#include <functional>
#include <fstream>
#include <sstream>
#include <zlib.h>
#include "Base.h"
#include "Macro.h"

#if !defined(_WIN32)
    #include <poll.h>
#endif

class HttpClient
{
public:
    enum ErrorType {
        ErrorNone,
        ConnectFailed,
        RequestInvalid,
        ReplyInvalid,
        DataAbnormal,
        NotImplement,
        LinkInterrupt,
        LinkZombie,
        TaskFinished,
        ErrorTypeEnd
    };

    HttpClient();
    virtual ~HttpClient();

    void SetResource(const char *url, const char *referer, const char *cookie);
    void SetStorage(const char *filepath);
    void SetListener(const std::function<void(const void *, size_t)> &listener);
    void SetKeepAlive(bool is_keep_alive) { is_keep_alive_ = is_keep_alive; }
    void SetUserData(const void *user_data) { user_data_ = user_data; }
    void SetTimeout(time_t active_timeout) { active_timeout_ = active_timeout; }

    bool RegisterObserver(struct pollfd &sockfd);
    void HandleExceptable();
    void HandleReadable();
    void HandleWritable();
    void CheckTimeout();

    virtual void ResetStatus();
    void Reset();

    int64 filesize() const { return filesize_; }
    uint64 filepos() const { return filepos_; }
    SOCKET sockfd() const { return sock_; }
    ErrorType error() const { return error_; }
    bool is_keep_alive() const { return is_keep_alive_; }
    const void *user_data() const { return user_data_; }
    const std::string &url() const { return url_; }
    const std::string &filepath() const { return filepath_; }
    const std::string &filedata() const { return filedata_; }

    static std::string EncodeUrl(const std::string& data);

protected:
    virtual void ProvideBody(std::string &body) {}
    virtual std::string BuildHeader(
        const char *path, const std::string &host,
        const std::string &referer, const std::string &cookie);

    void set_error(ErrorType error) { error_ = error; }

private:
    typedef enum {
        HTTP_COMPRESS_GZIP,
        HTTP_COMPRESS_DEFLATE,
        HTTP_COMPRESS_COMPRESS,
        HTTP_COMPRESS_IDENTITY
    } CompressMode;

    bool Connect();
    bool BuildHeader();
    void ProvideBody();
    void StartRedirect();

    void ScanHeader(const char *reply);
    void ParseHeader();
    void ParseBody();

    bool TryDigestData(const void *data, size_t size);
    bool InflateData(const void *data, size_t size);
    void WriteData(const void *data, size_t size);
    void DiscardData();

    bool IsFinishTask(bool is_connection_closed) const;
    void FinishTask();

    std::string url_;
    std::string referer_;
    std::string cookie_;
    std::string location_;

    std::function<void(const void *, size_t)> listener_;
    std::string filepath_;
    std::ofstream ofstream_;
    std::string filedata_;
    std::ostringstream osstream_;
    int64 filesize_;
    uint64 filepos_;

    bool is_requested_;
    bool is_replyed_;
    std::string request_;
    std::string reply_;

    bool is_chunked_;
    bool is_chunking_;
    uint32 chunk_size_;
    uint32 chunk_pos_;

    z_stream data_stream_;
    CompressMode compress_mode_;
    bool is_dummy_stream_initialised_;
    bool is_stream_initialized_;
    bool is_stream_ended_;

    SOCKET sock_;
    ErrorType error_;
    ErrorType forward_error_;

    bool is_keep_alive_;
    bool is_redirected_;
    const void *user_data_;
    time_t active_timeout_;
    time_t active_stamp_;
};
