#pragma once

#include "Logger.h"

#define DBGASSERT(expr) \
    do { \
        if (!(expr)) { \
            ELOG("DBGASSERT(%s).", #expr); \
        } \
    } while (0)
