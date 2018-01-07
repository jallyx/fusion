#pragma once

#include <zlib.h>

namespace zlib {

class DeflateStream
{
public:
    DeflateStream();
    ~DeflateStream();

    bool Deflate(const void *in, size_t &inlen, void *out, size_t &outlen);
    bool Flush(void *out, size_t &outlen);
    void Reset();

private:
    z_stream stream_;
};

class InflateStream
{
public:
    InflateStream();
    ~InflateStream();

    bool Inflate(const void *in, size_t &inlen, void *out, size_t &outlen);
    void Reset();

private:
    z_stream stream_;
};

}
