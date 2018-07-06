#include "ZlibStream.h"
#include "Logger.h"
#include <string.h>

namespace zlib {

DeflateStream::DeflateStream()
{
    memset(&stream_, 0, sizeof(stream_));
    int ret = deflateInit2(&stream_, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        ELOG("deflateInit2() Fatal Error %d.", ret);
    }
}

DeflateStream::~DeflateStream()
{
    int ret = deflateEnd(&stream_);
    if (ret != Z_OK && ret != Z_DATA_ERROR) {
        WLOG("deflateEnd() Has Error %d.", ret);
    }
}

bool DeflateStream::Deflate(const void *in, size_t &inlen, void *out, size_t &outlen)
{
    stream_.next_in = (Bytef*)in;
    stream_.avail_in = inlen;
    stream_.next_out = (Bytef*)out;
    stream_.avail_out = outlen;
    int ret = deflate(&stream_, Z_NO_FLUSH);
    if (ret != Z_OK) {
        WLOG("deflate(Z_NO_FLUSH) Has Error %d.", ret);
        return false;
    }
    inlen -= stream_.avail_in;
    outlen -= stream_.avail_out;
    return true;
}

bool DeflateStream::Flush(void *out, size_t &outlen)
{
    stream_.next_in = (Bytef*)"";
    stream_.avail_in = 0;
    stream_.next_out = (Bytef*)out;
    stream_.avail_out = outlen;
    int ret = deflate(&stream_, Z_SYNC_FLUSH);
    if (ret != Z_OK) {
        WLOG("deflate(Z_SYNC_FLUSH) Has Error %d.", ret);
        return false;
    }
    outlen -= stream_.avail_out;
    return true;
}

void DeflateStream::Reset()
{
    int ret = deflateReset(&stream_);
    if (ret != Z_OK) {
        WLOG("deflateReset() Has Error %d.", ret);
    }
}


InflateStream::InflateStream()
{
    memset(&stream_, 0, sizeof(stream_));
    int ret = inflateInit2(&stream_, -15);
    if (ret != Z_OK) {
        ELOG("inflateInit2() Fatal Error %d.", ret);
    }
}

InflateStream::~InflateStream()
{
    int ret = inflateEnd(&stream_);
    if (ret != Z_OK) {
        WLOG("inflateEnd() Has Error %d.", ret);
    }
}

bool InflateStream::Inflate(const void *in, size_t &inlen, void *out, size_t &outlen)
{
    stream_.next_in = (Bytef*)in;
    stream_.avail_in = inlen;
    stream_.next_out = (Bytef*)out;
    stream_.avail_out = outlen;
    int ret = inflate(&stream_, Z_SYNC_FLUSH);
    if (ret != Z_OK) {
        WLOG("inflate(Z_SYNC_FLUSH) Has Error %d.", ret);
        return false;
    }
    inlen -= stream_.avail_in;
    outlen -= stream_.avail_out;
    return true;
}

void InflateStream::Reset()
{
    int ret = inflateReset(&stream_);
    if (ret != Z_OK) {
        WLOG("inflateReset() Has Error %d.", ret);
    }
}

}
