#pragma once

#include "HttpClient.h"
#include <list>

class HttpPost : public HttpClient
{
public:
    enum PostErrorType
    {
        FileRestricted = ErrorTypeEnd,
        FileAbnormal,
        PostErrorTypeEnd
    };

    HttpPost();
    virtual ~HttpPost();

    void AppendPostValue(const char *name, const char *value);
    void AppendPostFile(const char *name, const char *filepath,
        const char *type = nullptr);
    void AppendPostBlock(const char *name, const char *filename,
        const std::string &filedata, const char *type = nullptr);

    virtual void ResetStatus();

    bool is_uploading_file() { return source_stream_.is_open(); }
    uint64 get_source_filesize() const { return source_filesize_; }
    uint64 get_source_filepos() const { return source_filepos_; }

protected:
    virtual void ProvideBody(std::string &body);
    virtual std::string BuildHeader(
        const char *path, const std::string &host,
        const std::string &referer, const std::string &cookie);

private:
    class FormPart {
    public:
        FormPart(bool isready, bool isfile)
            : isready_(isready), isfile_(isfile)
            , isdoing_(false)
        {}
        std::string header_;
        std::string value_;
        bool isready_;
        bool isfile_;
        bool isdoing_;
    };

    uint64 CalcContentLength() const;
    uint64 CalcContentLengthNormal() const;
    uint64 CalcContentLengthSimple() const;

    void ReadyBuildHeader();
    void ReadyBuildHeaderNormal();
    void ReadyBuildHeaderSimple();

    void ProvideBodyNormal(std::string &body);
    void ProvideBodySimple(std::string &body);

    bool OpenUploadFile(const char *filepath);
    void CloseUploadFile();

    static uint64 CalcFileSize(const char *filepath);
    static std::string EscapeFieldName(const char *data);

    char form_boundary_[31 + 1];
    bool first_boundary_;

    bool is_multipart_;
    std::list<FormPart> form_data_;

    std::ifstream source_stream_;
    uint64 source_filesize_;
    uint64 source_filepos_;
};
