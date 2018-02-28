#pragma once

#include <assert.h>
#include <string.h>
#include <algorithm>
#include <string>
#include "Base.h"
#include "Exception.h"
#include "noncopyable.h"

class INetStream : public noncopyable
{
public:
    INetStream()
        : internal_(nullptr)
        , buffer_(nullptr)
        , size_(0)
        , rpos_(0)
        , wpos_(0)
    {}
    virtual ~INetStream() {
        DeleteBuffer();
    }

    size_t GetReadPos() const { return rpos_; }
    size_t GetWritePos() const { return wpos_; }
    size_t GetTotalSize() const { return wpos_; }
    size_t GetBufferSize() const { return size_; }
    size_t GetReadableSize() const { return wpos_ - rpos_; }
    size_t GetWritableSize() const { return size_ - wpos_; }
    bool IsBufferEmpty() const {return wpos_ == 0; }
    bool IsReadableEmpty() const { return rpos_ >= wpos_; }
    const char *GetBuffer() const { return buffer_; }
    const char *GetReadableBuffer() const { return buffer_ + rpos_; }

    void ResetReadPos() { rpos_ = 0; }
    void AdjustReadPos(ssize_t size) { rpos_ += size; }
    void SkipReadPosToEnd() { rpos_ = wpos_; }

    std::string CastBufferString() const {
        return std::string(buffer_, wpos_);
    }
    std::string CastReadableString() const {
        return std::string(buffer_ + rpos_, wpos_ - rpos_);
    }

    void Clear() {
        rpos_ = wpos_ = 0;
    }

    void Erlarge(size_t size) {
        assert(size >= wpos_ && "erlarge fatal error.");
        CheckSize(size);
        wpos_ = size;
    }
    void Shrink(size_t size) {
        assert(size <= wpos_ && "shrink fatal error.");
        wpos_ = size;
        rpos_ = std::min(rpos_, size);
    }

    INetStream &Append(const void *data, size_t size) {
        if (size != 0) WriteStream(data, size);
        return *this;
    }
    INetStream &Take(void *data, size_t size) {
        if (size != 0) ReadStream(data, size);
        return *this;
    }

    INetStream &operator<<(bool v)
    { v=(*(u8*)(&v))!=0; WriteStream(&v, sizeof(v)); return *this; }
    INetStream &operator>>(bool &v)
    { ReadStream(&v, sizeof(v)); v=(*(u8*)(&v))!=0; return *this; }

#define STREAM_CONVERTER(TYPE) \
    INetStream &operator<<(TYPE v) \
    { WriteStream(&v, sizeof(v)); return *this; } \
    INetStream &operator>>(TYPE &v) \
    { ReadStream(&v, sizeof(v)); return *this; }
    STREAM_CONVERTER(char)
    STREAM_CONVERTER(int8)
    STREAM_CONVERTER(uint8)
    STREAM_CONVERTER(int16)
    STREAM_CONVERTER(uint16)
    STREAM_CONVERTER(int32)
    STREAM_CONVERTER(uint32)
    STREAM_CONVERTER(int64)
    STREAM_CONVERTER(uint64)
#undef STREAM_CONVERTER

#define STREAM_CONVERTER(TYPE) \
    INetStream &operator<<(TYPE v) { \
        if (std::isnan(v)) { THROW_EXCEPTION(NetStreamException()); } \
        WriteStream(&v, sizeof(v)); return *this; } \
    INetStream &operator>>(TYPE &v) { \
        ReadStream(&v, sizeof(v)); if (std::isnan(v)) { \
        THROW_EXCEPTION(NetStreamException()); } return *this; }
    STREAM_CONVERTER(float)
    STREAM_CONVERTER(double)
#undef STREAM_CONVERTER

    INetStream &operator<<(const std::string &s) {
        Write<uint16>((uint16)s.size());
        if (!s.empty()) WriteStream(s.data(), s.size());
        return *this;
    }
    INetStream &operator>>(std::string &s) {
        s.resize(Read<uint16>());
        if (!s.empty()) ReadStream((void*)s.data(), s.size());
        return *this;
    }

    INetStream &operator<<(const char *s) {
        size_t size = strlen(s); Write<uint16>((uint16)size);
        if (size != 0) WriteStream(s, size);
        return *this;
    }

    void WriteString(const void *data, size_t size) {
        Write<uint16>((uint16)size);
        if (size != 0) WriteStream(data, size);
    }

    template <typename T> void Write(T v) {
        operator<<(v);
    }
    template <typename T> T Read() {
        T v; operator>>(v); return v;
    }

    template <typename T> T Peek() {
        size_t rpos = rpos_; T v = Read<T>();
        rpos_ = rpos; return v;
    }

    template <typename T> size_t Placeholder(T v) {
        size_t wpos = wpos_; operator<<(v); return wpos;
    }

    template <typename T> void Put(size_t pos, T v) {
        assert(pos <= wpos_ && "put position fatal error.");
        size_t wpos = wpos_; wpos_ = pos; operator<<(v);
        wpos_ = std::max(wpos, wpos_);
    }

protected:
    void InitInternalBuffer(char *buffer, size_t size) {
        assert(buffer_ == nullptr && "init fatal error.");
        internal_ = buffer_ = buffer;
        size_ = size;
    }

private:
    void CheckSize(size_t size) {
        if (size > size_) {
            Resize(size);
        }
    }
    void Resize(size_t size) {
        if (size > size_) {
            size_t newSize = std::max(size, size_ << 1);
            char *newBuffer = new char[newSize];
            if (wpos_ != 0) {
                memcpy(newBuffer, buffer_, wpos_);
            }
            if (buffer_ != nullptr) {
                DeleteBuffer();
            }
            buffer_ = newBuffer;
            size_ = newSize;
        }
    }
    void DeleteBuffer() {
        if (buffer_ != nullptr && buffer_ != internal_) {
            delete [] buffer_;
        }
    }

    void WriteStream(const void *data, size_t size) {
        CheckSize(wpos_ + size);
        memcpy(buffer_ + wpos_, data, size);
        wpos_ += size;
    }
    void ReadStream(void *data, size_t size) {
        if (rpos_ + size <= wpos_) {
            memcpy(data, buffer_ + rpos_, size);
            rpos_ += size;
        } else {
            THROW_EXCEPTION(NetStreamException());
        }
    }

    const char *internal_;
    char *buffer_;
    size_t size_, rpos_, wpos_;
};
