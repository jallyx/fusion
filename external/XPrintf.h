#pragma once

#include <string>
#include "NetStream.h"
#include "TextPacker.h"

// %[flags][width][.precision][length]specifier
// length:
// int8,uint8 hh;
// int16,uint16 h;
// int64,uint64 ll;
// double l;
// strid n;
// specifier:
// diuoxXfFeEgGcs

std::string NetStream_Printf(const char *fmt, INetStream &args);
std::string Text_Printf(const char *fmt, TextUnpacker &args);

extern const char *I18N_StrID(uint32 strid);
