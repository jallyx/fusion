#pragma once

#include <ostream>
#include "Base.h"

void WriteTEXT(std::ostream &stream, const char *text);
std::string ReadTEXT(const char *&ptr);

bool ReadBOOL(const char *&ptr);
float ReadFLOAT(const char *&ptr);
double ReadDOUBLE(const char *&ptr);
int32 ReadINT32(const char *&ptr, int base = 0);
int64 ReadINT64(const char *&ptr, int base = 0);
uint32 ReadUINT32(const char *&ptr, int base = 0);
uint64 ReadUINT64(const char *&ptr, int base = 0);
