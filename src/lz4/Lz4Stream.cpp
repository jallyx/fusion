#include "Lz4Stream.h"
#include "Logger.h"
#include <string.h>
#include <algorithm>

namespace lz4 {

static const size_t MAX_SRC_SIZE = 64*1024;

CompressStream::CompressStream()
: cctx_(nullptr)
, dst_(nullptr)
, i_(0)
, n_(0)
, cap_(0)
, flush_(true)
{
    memset(&prefs_, 0, sizeof(prefs_));
    prefs_.frameInfo.blockSizeID = LZ4F_max64KB;
    dst_ = new char[cap_ = LZ4F_compressBound(MAX_SRC_SIZE, &prefs_)];
    LZ4F_errorCode_t ret = LZ4F_createCompressionContext(&cctx_, LZ4F_VERSION);
    if (LZ4F_isError(ret)) {
        ELOG("LZ4F_createCompressionContext() Fatal Error %s.", LZ4F_getErrorName(ret));
        return;
    }
    n_ = LZ4F_compressBegin(cctx_, dst_, cap_, &prefs_);
    if (LZ4F_isError(n_)) {
        ELOG("LZ4F_compressBegin() Fatal Error %s.", LZ4F_getErrorName(n_));
        return;
    }
}

CompressStream::~CompressStream()
{
    delete[] dst_;
    LZ4F_errorCode_t ret = LZ4F_freeCompressionContext(cctx_);
    if (LZ4F_isError(ret)) {
        WLOG("LZ4F_freeCompressionContext() Has Error %s.", LZ4F_getErrorName(ret));
    }
}

bool CompressStream::Compress(const void *in, size_t &inlen, void *out, size_t &outlen)
{
    size_t in_digest = 0, out_avail = 0;
    for (;; i_ = 0, flush_ = false) {
        if (i_ < n_) {
            const size_t avail = std::min(n_ - i_, outlen - out_avail);
            memcpy((char *)out + out_avail, dst_ + i_, avail);
            out_avail += avail;
            i_ += avail;
        }
        if (out_avail >= outlen || in_digest >= inlen) {
            outlen = out_avail, inlen = in_digest;
            return true;
        }
        const size_t avail = std::min(MAX_SRC_SIZE, inlen - in_digest);
        n_ = LZ4F_compressUpdate(cctx_, dst_, cap_, (const char *)in + in_digest, avail, nullptr);
        if (LZ4F_isError(n_)) {
            WLOG("LZ4F_compressUpdate() Has Error %s.", LZ4F_getErrorName(n_));
            return false;
        }
        in_digest += avail;
    }
}

bool CompressStream::Flush(void *out, size_t &outlen)
{
    size_t out_avail = 0;
    for (;; i_ = 0, flush_ = true) {
        if (i_ < n_) {
            const size_t avail = std::min(n_ - i_, outlen - out_avail);
            memcpy((char *)out + out_avail, dst_ + i_, avail);
            out_avail += avail;
            i_ += avail;
        }
        if (out_avail >= outlen || flush_) {
            outlen = out_avail;
            return true;
        }
        n_ = LZ4F_flush(cctx_, dst_, cap_, nullptr);
        if (LZ4F_isError(n_)) {
            WLOG("LZ4F_flush() Has Error %s.", LZ4F_getErrorName(n_));
            return false;
        }
    }
}


DecompressStream::DecompressStream()
: dctx_(nullptr)
{
    LZ4F_errorCode_t ret = LZ4F_createDecompressionContext(&dctx_, LZ4F_VERSION);
    if (LZ4F_isError(ret)) {
        ELOG("LZ4F_createDecompressionContext() Fatal Error %s.", LZ4F_getErrorName(ret));
        return;
    }
}

DecompressStream::~DecompressStream()
{
    LZ4F_errorCode_t ret = LZ4F_freeDecompressionContext(dctx_);
    if (LZ4F_isError(ret)) {
        WLOG("LZ4F_freeDecompressionContext() Has Error %s.", LZ4F_getErrorName(ret));
    }
}

bool DecompressStream::Decompress(const void *in, size_t &inlen, void *out, size_t &outlen)
{
    size_t ret = LZ4F_decompress(dctx_, out, &outlen, in, &inlen, nullptr);
    if (LZ4F_isError(ret)) {
        WLOG("LZ4F_decompress() Has Error %s.", LZ4F_getErrorName(ret));
        return false;
    }
    return true;
}

}
