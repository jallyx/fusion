#pragma once

#include <sstream>
#include "TextUtils.h"

class LogPacker
{
public:
    template <typename T>
    LogPacker &operator<<(T v);

    void PutDelimiter(char delimiter = ',') {
        stream.seekp(-1, std::ios::cur).put(delimiter);
    }

    std::string str() const { return stream.str(); }
    void str(const std::string &s) { stream.str(s); }

private:
    std::ostringstream stream;
};

class TextPacker
{
public:
    template <typename T>
    TextPacker &operator<<(T v);

    void PutAnchor(char anchor = ';') {
        stream.put(anchor);
    }
    void PutDelimiter(char delimiter = ',') {
        stream.seekp(-1, std::ios::cur).put(delimiter);
    }

    std::string str() const { return stream.str(); }
    void str(const std::string &s) { stream.str(s); }

private:
    std::ostringstream stream;
};

class TextUnpacker
{
public:
    TextUnpacker(const char *s)
        : ptr(s)
    {}

    template <typename T>
    TextUnpacker &operator>>(T &v);

    template <typename T> T Unpack() {
        T v; operator>>(v); return v;
    }

    bool IsAnchor(char anchor = ';') {
        return *ptr == anchor ? (++ptr, true) : false;
    }
    bool IsDelimiter(char delimiter = ',') const {
        return *(ptr - 1) == delimiter;
    }

    const char *GetIterator() { return ptr; }
    void SetIterator(const char *p) { ptr = p; }
    bool IsEmpty() const { return *ptr == '\0'; }

private:
    const char *ptr;
};

template <typename T>
LogPacker &LogPacker::operator<<(T v)
{
    stream << v << ',';
    return *this;
}
template <>
inline LogPacker &LogPacker::operator<<(int8 v)
{
    stream << (int)v << ',';
    return *this;
}
template <>
inline LogPacker &LogPacker::operator<<(uint8 v)
{
    stream << (int)v << ',';
    return *this;
}

template <typename T>
TextPacker &TextPacker::operator<<(T v)
{
    stream << v << ',';
    return *this;
}
template <>
inline TextPacker &TextPacker::operator<<(int8 v)
{
    stream << (int)v << ',';
    return *this;
}
template <>
inline TextPacker &TextPacker::operator<<(uint8 v)
{
    stream << (int)v << ',';
    return *this;
}
template <>
inline TextPacker &TextPacker::operator<<(char *s)
{
    WriteTEXT(stream, s);
    return *this;
}
template <>
inline TextPacker &TextPacker::operator<<(const char *s)
{
    WriteTEXT(stream, s);
    return *this;
}
template <>
inline TextPacker &TextPacker::operator<<(std::string s)
{
    return operator<<(s.c_str());
}

template <typename T>
TextUnpacker &TextUnpacker::operator>>(T &v)
{
    v = static_cast<T>(ReadINT32(ptr));
    return *this;
}
template <>
inline TextUnpacker &TextUnpacker::operator>>(uint32 &v)
{
    v = ReadUINT32(ptr);
    return *this;
}
template <>
inline TextUnpacker &TextUnpacker::operator>>(int64 &v)
{
    v = ReadINT64(ptr);
    return *this;
}
template <>
inline TextUnpacker &TextUnpacker::operator>>(uint64 &v)
{
    v = ReadUINT64(ptr);
    return *this;
}
template <>
inline TextUnpacker &TextUnpacker::operator>>(bool &v)
{
    v = ReadBOOL(ptr);
    return *this;
}
template <>
inline TextUnpacker &TextUnpacker::operator>>(float &v)
{
    v = ReadFLOAT(ptr);
    return *this;
}
template <>
inline TextUnpacker &TextUnpacker::operator>>(double &v)
{
    v = ReadDOUBLE(ptr);
    return *this;
}
template <>
inline TextUnpacker &TextUnpacker::operator>>(std::string &s)
{
    s = ReadTEXT(ptr);
    return *this;
}
