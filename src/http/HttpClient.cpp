#include "HttpClient.h"
#include <assert.h>
#include <string.h>
#include "Debugger.h"
#include "OS.h"

HttpClient::HttpClient()
: easy_handle_(nullptr)
, error_(ErrorNone)
, filesize_(-1)
, filepos_(0)
{
}

HttpClient::~HttpClient()
{
    if (easy_handle_ != nullptr) {
        curl_easy_cleanup(easy_handle_);
    }
}

void HttpClient::SetResource(const char *url, const char *referer, const char *cookie)
{
    assert(url != nullptr && *url != '\0');
    url_.assign(url);
    if (referer != nullptr && *referer != '\0') {
        referer_.assign(referer);
    }
    if (cookie != nullptr && *cookie != '\0') {
        cookie_.assign(cookie);
    }
}

void HttpClient::SetStorage(const char *filepath)
{
    if (filepath != nullptr && *filepath != '\0') {
        OS::CreateFilePath(filepath);
        filepath_.assign(filepath);
        ofstream_.open(filepath,
            std::ios::app | std::ios::ate | std::ios::binary);
        if (ofstream_.is_open()) {
            filepos_ = ofstream_.tellp();
        }
    }
}

void HttpClient::SetListener(
        const std::function<void(const void *, size_t)> &listener)
{
    listener_ = listener;
}

void HttpClient::Register(CURLM *multi_handle)
{
    if ((easy_handle_ = curl_easy_init()) == nullptr) {
        error_ = InitFailed;
        return;
    }

    curl_easy_setopt(easy_handle_, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(easy_handle_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(easy_handle_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy_handle_, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(easy_handle_, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(easy_handle_, CURLOPT_ACCEPT_ENCODING, "");

    curl_easy_setopt(easy_handle_, CURLOPT_HEADERFUNCTION, fnHeader);
    curl_easy_setopt(easy_handle_, CURLOPT_HEADERDATA, this);

    curl_easy_setopt(easy_handle_, CURLOPT_WRITEFUNCTION, fnWrite);
    curl_easy_setopt(easy_handle_, CURLOPT_WRITEDATA, this);

    if (!referer_.empty()) {
        curl_easy_setopt(easy_handle_, CURLOPT_REFERER, referer_.c_str());
    }
    if (!cookie_.empty()) {
        curl_easy_setopt(easy_handle_, CURLOPT_COOKIE, cookie_.c_str());
    }

    if (filepos_ != 0) {
        curl_easy_setopt(easy_handle_, CURLOPT_RESUME_FROM_LARGE, curl_off_t(filepos_));
    }

    if (curl_multi_add_handle(multi_handle, easy_handle_) != CURLM_OK) {
        error_ = InitFailed;
        return;
    }

    curl_easy_setopt(easy_handle_, CURLOPT_PRIVATE, this);
}

void HttpClient::Unregister(CURLM *multi_handle)
{
    if (easy_handle_ != nullptr) {
        curl_multi_remove_handle(multi_handle, easy_handle_);
    }
}

void HttpClient::Done(CURLcode result)
{
    if (result == CURLE_OK) {
        DBGASSERT(filesize_ == -1 || size_t(filesize_) == filepos_);
        error_ = TaskFinished;
    } else {
        error_ = TaskFailed;
    }

    if (!listener_) {
        if (!filepath_.empty()) {
            if (ofstream_.is_open()) {
                ofstream_.close();
            }
        } else {
            filedata_ = osstream_.str();
            osstream_.str("");
        }
    }
}

void HttpClient::TruncFile()
{
    if (filepos_ != 0) {
        filepos_ = 0;
    } else {
        return;
    }

    if (!listener_) {
        if (!filepath_.empty()) {
            if (ofstream_.is_open()) {
                ofstream_.close();
                ofstream_.open(filepath_.c_str(),
                    std::ios::trunc | std::ios::binary);
            }
        }
    }
}

size_t HttpClient::OnHeader(char *buffer, size_t size, size_t nitems)
{
    const size_t n = size * nitems;

    if (n == 2 && memcmp(buffer, "\r\n", 2) == 0) {
        long response_code = 0;
        curl_easy_getinfo(easy_handle_, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200 || response_code == 416) {
            TruncFile();
        }
        double cl = -1;
        curl_easy_getinfo(easy_handle_, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl);
        if (cl != -1) {
            switch (response_code) {
            case 200:
                filesize_ = ssize_t(cl);
                break;
            case 206:
                filesize_ = filepos_ + size_t(cl);
                break;
            }
        }
    }

    return n;
}

size_t HttpClient::fnHeader(char *buffer, size_t size, size_t nitems, void *userdata)
{
    return ((HttpClient*)userdata)->OnHeader(buffer, size, nitems);
}

size_t HttpClient::OnWrite(char *ptr, size_t size, size_t nmemb)
{
    const size_t n = size * nmemb;
    filepos_ += n;

    if (listener_) {
        listener_(ptr, n);
    } else if (!filepath_.empty()) {
        ofstream_.write(ptr, n);
    } else {
        osstream_.write(ptr, n);
    }

    return n;
}

size_t HttpClient::fnWrite(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    return ((HttpClient*)userdata)->OnWrite(ptr, size, nmemb);
}
