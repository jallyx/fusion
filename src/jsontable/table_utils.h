#pragma once

#include <sstream>
#include "NetStream.h"
#include "table_interface.h"

template<typename T> inline void LoadFromBinary(T &entity, const std::string &binary) {
    std::istringstream stream(binary);
    LoadFromStream(entity, stream);
}
template<typename T> inline std::string SaveToBinary(const T &entity) {
    std::ostringstream stream;
    SaveToStream(entity, stream);
    return stream.str();
}

class streambuf_ConstBuffer : public std::streambuf {
public:
    streambuf_ConstBuffer(const void *dataptr, const void *endptr) {
        setg((char *)dataptr, (char *)dataptr, (char *)endptr);
    }
private:
    virtual std::streamsize xsgetn(char *s, std::streamsize n) {
        std::streamsize size = std::min(n, in_avail());
        std::copy(gptr(), gptr() + size, s);
        gbump((int)size);
        return size;
    }
};
class istream_ConstBuffer : public std::istream {
public:
    istream_ConstBuffer(const void *dataptr, const void *endptr)
        : std::istream(nullptr), streambuf_(dataptr, endptr)
    { this->init(&streambuf_); }
private:
    streambuf_ConstBuffer streambuf_;
};
template <typename T>
inline void LoadFromConstBuffer(T &entity, const void *dataptr, const void *endptr) {
    istream_ConstBuffer stream(dataptr, endptr);
    LoadFromStream(entity, stream);
}
template <typename T>
inline void LoadFromConstBuffer(T &entity, const void *dataptr, size_t size) {
    LoadFromConstBuffer(entity, dataptr, (const char *)dataptr + size);
}

class streambuf_INetStream : public std::streambuf {
public:
    streambuf_INetStream(INetStream &stream) : stream_(stream) {}
private:
    virtual std::streamsize showmanyc() { return stream_.GetReadableSize(); }
    virtual std::streamsize xsgetn(char *s, std::streamsize n) {
        std::streamsize size = std::min(n, in_avail());
        stream_.Take(s, (size_t)size);
        return size;
    }
    virtual std::streamsize xsputn(const char *s, std::streamsize n) {
        stream_.Append(s, (size_t)n);
        return n;
    }
    INetStream &stream_;
};
class iostream_INetStream : public std::istream, public std::ostream {
public:
    iostream_INetStream(INetStream &stream)
        : std::istream(nullptr), std::ostream(nullptr), streambuf_(stream)
    { this->init(&streambuf_); }
private:
    streambuf_INetStream streambuf_;
};
template <typename T>
inline void LoadFromINetStream(T &entity, INetStream &buffer) {
    iostream_INetStream stream(buffer);
    LoadFromStream(entity, stream);
}
template <typename T>
inline void SaveToINetStream(const T &entity, INetStream &buffer) {
    iostream_INetStream stream(buffer);
    SaveToStream(entity, stream);
}
