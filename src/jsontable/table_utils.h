#pragma once

#include <sstream>
#include "NetStream.h"
#include "table_controller.h"

template<typename T> inline void LoadFromBinary(T &entity, const std::string &binary) {
    std::istringstream stream(binary);
    LoadFromStream(entity, stream);
}
template<typename T> inline std::string SaveToBinary(const T &entity) {
    std::ostringstream stream;
    SaveToStream(entity, stream);
    return stream.str();
}

class streambuf_StaticBuffer : public std::streambuf {
public:
    streambuf_StaticBuffer(const void *dataptr, const void *endptr) {
        setg((char *)dataptr, (char *)dataptr, (char *)endptr);
    }
private:
    virtual std::streamsize xsgetn(char *s, std::streamsize n) {
        std::streamsize size = std::min(n, in_avail());
        std::copy(gptr(), gptr() + size, s);
        gbump(size);
        return size;
    }
};
class istream_StaticBuffer : std::istream {
public:
    istream_StaticBuffer(const void *dataptr, const void *endptr)
        : std::istream(&streambuf_), streambuf_(dataptr, endptr)
    {}
private:
    streambuf_StaticBuffer streambuf_;
};
template <typename T>
inline void LoadFromStaticBuffer(T &entity, const void *dataptr, const void *endptr) {
    istream_StaticBuffer stream(dataptr, endptr);
    LoadFromStream(entity, stream);
}

class streambuf_INetStream : public std::streambuf {
public:
    streambuf_INetStream(INetStream &stream) : stream_(stream) {}
private:
    virtual std::streamsize showmanyc() { return stream_.GetReadableSize(); }
    virtual int_type underflow() {
        if (!stream_.IsReadableEmpty()) return stream_.Peek<char>();
        return std::char_traits<char>::eof();
    }
    virtual int_type uflow() {
        if (!stream_.IsReadableEmpty()) return stream_.Read<char>();
        return std::char_traits<char>::eof();
    }
    virtual std::streamsize xsgetn(char *s, std::streamsize n) {
        std::streamsize size = std::min(n, in_avail());
        stream_.Take(s, size);
        return size;
    }
    virtual int_type overflow(int_type c) {
        stream_.Write((char)c);
        return c;
    }
    virtual std::streamsize xsputn(const char *s, std::streamsize n) {
        stream_.Append(s, n);
        return n;
    }
    INetStream &stream_;
};
class istream_INetStream : public std::istream {
public:
    istream_INetStream(INetStream &stream)
        : std::istream(&streambuf_), streambuf_(stream)
    {}
private:
    streambuf_INetStream streambuf_;
};
class ostream_INetStream : public std::ostream {
public:
    ostream_INetStream(INetStream &stream)
        : std::ostream(&streambuf_), streambuf_(stream)
    {}
private:
    streambuf_INetStream streambuf_;
};
template <typename T>
inline void LoadFromINetStream(T &entity, INetStream &stream) {
    istream_INetStream stream(stream);
    LoadFromStream(entity, stream);
}
template <typename T>
inline void SaveToINetStream(T &entity, INetStream &stream) {
    ostream_INetStream stream(stream);
    SaveToStream(entity, stream);
}
