#pragma once

#include <curl/curl.h>
#include <functional>
#include <fstream>
#include <sstream>
#include "Base.h"

class HttpClient
{
public:
    enum ErrorType {
        ErrorNone,
        InitFailed,
        TaskFailed,
        TaskFinished,
        ErrorTypeEnd
    };

    HttpClient();
    virtual ~HttpClient();

    void SetResource(const char *url, const char *referer, const char *cookie);
    void SetStorage(const char *filepath);
    void SetListener(const std::function<void(const void *, size_t)> &listener);

    void Register(CURLM *multi_handle);
    void Unregister(CURLM *multi_handle);
    void Done(CURLcode result);

    ErrorType error() const { return error_; }
    int64 filesize() const { return filesize_; }
    uint64 filepos() const { return filepos_; }
    const std::string &url() const { return url_; }
    const std::string &filepath() const { return filepath_; }
    const std::string &filedata() const { return filedata_; }

private:
    void TruncFile();

    size_t OnHeader(char *buffer, size_t size, size_t nitems);
    static size_t fnHeader(char *buffer, size_t size, size_t nitems, void *userdata);

    size_t OnWrite(char *ptr, size_t size, size_t nmemb);
    static size_t fnWrite(char *ptr, size_t size, size_t nmemb, void *userdata);

    CURL *easy_handle_;
    ErrorType error_;

    std::string url_;
    std::string referer_;
    std::string cookie_;

    std::function<void(const void *, size_t)> listener_;
    std::string filepath_;
    std::ofstream ofstream_;
    std::string filedata_;
    std::ostringstream osstream_;

    int64 filesize_;
    uint64 filepos_;
};
