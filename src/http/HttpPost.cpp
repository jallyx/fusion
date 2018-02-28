#include "HttpPost.h"
#include <string.h>
#include "System.h"

#define USER_AGENT "9FANG MMORPG"
#define POST_CACHE_MIN 1024

#define FormBoundaryRefer \
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define FileTypeDefault \
    "application/octet-stream"

#define StreamPackValueHeader(stream,name) \
    do { \
        stream << "Content-Disposition: form-data; name=\"" << name << "\"\r\n"; \
        stream << "\r\n"; \
    } while (0)
#define StreamPackFileHeader(stream,name,filename,type) \
    do { \
        stream << "Content-Disposition: form-data; name=\"" << name << "\"; filename=\"" << filename << "\"\r\n"; \
        stream << "Content-Type: " << (type != nullptr && *type != '\0' ? type : FileTypeDefault) << "\r\n"; \
        stream << "\r\n"; \
    } while (0)

HttpPost::HttpPost()
: first_boundary_(false)
, is_multipart_(false)
, source_filesize_(0)
, source_filepos_(0)
{
}

HttpPost::~HttpPost()
{
}

void HttpPost::ResetStatus()
{
    if (source_stream_.is_open())
        source_stream_.close();

    form_data_.clear();
    source_stream_.clear();

    first_boundary_ = false;
    is_multipart_ = false;
    source_filesize_ = 0;
    source_filepos_ = 0;

    HttpClient::ResetStatus();
}

void HttpPost::AppendPostValue(const char *name, const char *value)
{
    form_data_.emplace_back(false, false);
    FormPart &form_part = form_data_.back();
    form_part.header_ = name;
    form_part.value_ = value;
}

void HttpPost::AppendPostFile(const char *name, const char *filepath, const char *type)
{
    const char *lastpath = strrchr(filepath, os::sep);
    const char *filename = lastpath != nullptr ? lastpath + 1 : filepath;

    std::ostringstream stream;
    StreamPackFileHeader(stream, EscapeFieldName(name), filename, type);

    form_data_.emplace_back(true, true);
    FormPart &form_part = form_data_.back();
    form_part.header_ = stream.str();
    form_part.value_ = filepath;

    is_multipart_ = true;
}

void HttpPost::AppendPostBlock(const char *name, const char *filename,
                               const std::string &filedata, const char *type)
{
    std::ostringstream stream;
    StreamPackFileHeader(stream, EscapeFieldName(name), filename, type);

    form_data_.emplace_back(true, false);
    FormPart &form_part = form_data_.back();
    form_part.header_ = stream.str();
    form_part.value_ = filedata;

    is_multipart_ = true;
}

void HttpPost::ProvideBody(std::string &body)
{
    is_multipart_ ? ProvideBodyNormal(body) : ProvideBodySimple(body);
}

void HttpPost::ReadyBuildHeader()
{
    is_multipart_ ? ReadyBuildHeaderNormal() : ReadyBuildHeaderSimple();
}

uint64 HttpPost::CalcContentLength() const
{
    return is_multipart_ ? CalcContentLengthNormal() : CalcContentLengthSimple();
}

std::string HttpPost::BuildHeader(
    const char *path, const std::string &host,
    const std::string &referer, const std::string &cookie)
{
    ReadyBuildHeader();
    std::ostringstream stream;
    stream << "POST " << path << " HTTP/1.1\r\n";
    stream << "HOST: " << host << "\r\n";
    stream << "User-Agent: " << USER_AGENT << "\r\n";
    stream << "Accept: text/html,*/*;q=0.8\r\n";
    stream << "Accept-Encoding: gzip, deflate\r\n";
    if (is_keep_alive())
        stream << "Connection: keep-alive\r\n";
    else
        stream << "Connection: close\r\n";
    if (is_multipart_)
        stream << "Content-Type: multipart/form-data; boundary=" << form_boundary_ << "\r\n";
    else
        stream << "Content-Type: application/x-www-form-urlencoded\r\n";
    stream << "Content-Length: " << CalcContentLength() << "\r\n";
    if (!referer.empty())
        stream << "Referer: " << referer << "\r\n";
    if (!cookie.empty())
        stream << "Cookie: " << cookie << "\r\n";
    stream << "\r\n";
    return stream.str();
}

uint64 HttpPost::CalcContentLengthNormal() const
{
    uint64 form_boundary_length = strlen(form_boundary_);
    uint64 content_length = 2 + form_boundary_length + 2 + 2;
    for (auto &form_part : form_data_) {
        content_length += form_part.header_.size();
        if (form_part.isfile_)
            content_length += CalcFileSize(form_part.value_.c_str());
        else
            content_length += form_part.value_.size();
        content_length += 2 + 2 + form_boundary_length + 2;
    }
    return content_length;
}

void HttpPost::ProvideBodyNormal(std::string &body)
{
    if (first_boundary_ && form_data_.empty())
        return;

    if (!first_boundary_) {
        if (!form_data_.empty()) {
            body.append("--").append(form_boundary_).append("\r\n");
        } else {
            body.append("--").append(form_boundary_).append("--\r\n");
        }
        first_boundary_ = true;
    }

    if (!form_data_.empty()) {
        FormPart &form_part = form_data_.front();
        if (!form_part.isdoing_) {
            if (form_part.isfile_) {
                if (!OpenUploadFile(form_part.value_.c_str())) {
                    set_error((ErrorType)FileRestricted);
                    return;
                }
            }
            body.append(form_part.header_);
            form_part.isdoing_ = true;
        } else {
            bool isdone = false;
            if (form_part.isfile_) {
                char buffer[POST_CACHE_MIN];
                source_stream_.read(buffer, sizeof(buffer));
                if (source_stream_.bad()) {
                    set_error((ErrorType)FileAbnormal);
                    return;
                }
                body.append(buffer, (size_t)source_stream_.gcount());
                source_filepos_ += source_stream_.gcount();
                if (source_stream_.fail() || source_stream_.eof()) {
                    if (source_filepos_ == source_filesize_) {
                        CloseUploadFile();
                        isdone = true;
                    } else {
                        set_error((ErrorType)FileAbnormal);
                        return;
                    }
                }
            } else {
                body.append(form_part.value_);
                isdone = true;
            }
            if (isdone) {
                form_data_.pop_front();
                if (!form_data_.empty()) {
                    body.append("\r\n--").append(form_boundary_).append("\r\n");
                } else {
                    body.append("\r\n--").append(form_boundary_).append("--\r\n");
                }
            }
        }
    }
}

void HttpPost::ReadyBuildHeaderNormal()
{
    form_boundary_[sizeof(form_boundary_) - 1] = '\0';
    for (size_t index = 0; index < sizeof(form_boundary_) - 1; ++index) {
        form_boundary_[index] =
            FormBoundaryRefer[System::Rand(0, sizeof(FormBoundaryRefer) - 1)];
    }

    for (auto &form_part : form_data_) {
        if (!form_part.isready_) {
            std::ostringstream stream;
            StreamPackValueHeader(stream,
                EscapeFieldName(form_part.header_.c_str()));
            form_part.header_ = stream.str();
            form_part.isready_ = true;
        }
    }
}

uint64 HttpPost::CalcContentLengthSimple() const
{
    return !form_data_.empty() ? form_data_.front().value_.size() : 0;
}

void HttpPost::ProvideBodySimple(std::string &body)
{
    if (!form_data_.empty()) {
        body.append(form_data_.front().value_);
        form_data_.clear();
    }
}

void HttpPost::ReadyBuildHeaderSimple()
{
    std::ostringstream stream;
    for (auto &form_part : form_data_) {
        stream << EncodeUrl(form_part.header_.c_str()) << '='
               << EncodeUrl(form_part.value_.c_str()) << '&';
    }
    if (!form_data_.empty()) {
        form_data_.erase(++form_data_.begin(), form_data_.end());
        FormPart &form_part = form_data_.front();
        form_part.value_ = stream.str();
        form_part.value_.erase(form_part.value_.size() - 1);
        form_part.header_.clear();
    }
}

bool HttpPost::OpenUploadFile(const char *filepath)
{
    source_stream_.open(filepath, std::ios::ate|std::ios::binary);
    if (!source_stream_.is_open())
        return false;

    source_filesize_ = source_stream_.tellg();
    source_filepos_ = 0;
    source_stream_.seekg(0, std::ios::beg);

    return true;
}

void HttpPost::CloseUploadFile()
{
    if (source_stream_.is_open()) {
        source_stream_.close();
    }
    source_stream_.clear();
}

uint64 HttpPost::CalcFileSize(const char *filepath)
{
    std::ifstream ifstream(filepath, std::ios::ate|std::ios::binary);
    return ifstream.is_open() ? (uint64)ifstream.tellg() : 0;
}

std::string HttpPost::EscapeFieldName(const char *data)
{
    std::string result;
    result.reserve(strlen(data) * 2);
    for (const char *ptr = data; *ptr != '\0'; ++ptr) {
        char value = *ptr;
        if (value == '"')
            result.append(1, '\\');
        result.append(1, value);
    }
    return result;
}
