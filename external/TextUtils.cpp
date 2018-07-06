#include "TextUtils.h"
#include <stdlib.h>
#include <string.h>

void WriteTEXT(std::ostream &stream, const char *text)
{
    size_t len = strlen(text);
    stream << len;
    if (len != 0)
        (stream << ':').write(text, len);
    stream << ',';
}

std::string ReadTEXT(const char *&ptr)
{
    std::string text;
    size_t len = ReadUINT32(ptr);
    if (len != 0) {
        const char *endptr = (const char *)memchr(ptr, '\0', len + 1);
        if (endptr != nullptr) {
            text.assign(ptr, endptr);
            ptr = endptr;
        } else {
            text.assign(ptr, len);
            ptr += len + 1;
        }
    }
    return text;
}

char ReadCHAR(const char *&ptr)
{
    const char ch = *ptr;
    ptr += ptr[0] != '\0' ? (ptr[1] != '\0' ? 2 : 1) : 0;
    return ch;
}

bool ReadBOOL(const char *&ptr)
{
    char *endptr = nullptr;
    long numeric = strtol(ptr, &endptr, 0);
    ptr = *endptr != '\0' ? endptr + 1 : endptr;
    return numeric != 0;
}

float ReadFLOAT(const char *&ptr)
{
    char *endptr = nullptr;
    float numeric = strtof(ptr, &endptr);
    ptr = *endptr != '\0' ? endptr + 1 : endptr;
    return numeric;
}

double ReadDOUBLE(const char *&ptr)
{
    char *endptr = nullptr;
    double numeric = strtod(ptr, &endptr);
    ptr = *endptr != '\0' ? endptr + 1 : endptr;
    return numeric;
}

int32 ReadINT32(const char *&ptr, int base)
{
    char *endptr = nullptr;
    int32 numeric = strtol(ptr, &endptr, base);
    ptr = *endptr != '\0' ? endptr + 1 : endptr;
    return numeric;
}

int64 ReadINT64(const char *&ptr, int base)
{
    char *endptr = nullptr;
    int64 numeric = strtoll(ptr, &endptr, base);
    ptr = *endptr != '\0' ? endptr + 1 : endptr;
    return numeric;
}

uint32 ReadUINT32(const char *&ptr, int base)
{
    char *endptr = nullptr;
    uint32 numeric = strtoul(ptr, &endptr, base);
    ptr = *endptr != '\0' ? endptr + 1 : endptr;
    return numeric;
}

uint64 ReadUINT64(const char *&ptr, int base)
{
    char *endptr = nullptr;
    uint64 numeric = strtoull(ptr, &endptr, base);
    ptr = *endptr != '\0' ? endptr + 1 : endptr;
    return numeric;
}
