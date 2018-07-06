#pragma once

#include <lz4frame.h>

namespace lz4 {

class CompressStream
{
public:
    CompressStream();
    ~CompressStream();

    bool Compress(const void *in, size_t &inlen, void *out, size_t &outlen);
    bool Flush(void *out, size_t &outlen);

private:
    LZ4F_preferences_t prefs_;
    LZ4F_cctx *cctx_;
    char *dst_;
    size_t i_, n_, cap_;
    bool flush_;
};

class DecompressStream
{
public:
    DecompressStream();
    ~DecompressStream();

    bool Decompress(const void *in, size_t &inlen, void *out, size_t &outlen);

private:
    LZ4F_dctx *dctx_;
};

}
