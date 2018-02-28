#pragma once

#include "Logger.h"

#if defined(_WIN32)
    #define ASSERT assert
#else
    #define ASSERT
#endif

#define DBGASSERT(expr) \
    do { \
        if (!(expr)) { \
            ELOG("DBGASSERT(%s).", #expr); \
        } \
    } while (0)

#define DBG_ASSERT_P0(cond,msg) \
    do { \
        if (!(cond)) { \
            ELOG(msg); \
        } \
    } while (0)

#define DBG_ASSERT_P1(cond,msg,p1) \
    do { \
        if (!(cond)) { \
            ELOG(msg,p1); \
        } \
    } while (0)

#define DBG_ASSERT_P2(cond,msg,p1,p2) \
    do { \
        if (!(cond)) { \
            ELOG(msg,p1,p2); \
        } \
    } while (0)

#define DBG_ASSERT_P3(cond,msg,p1,p2,p3) \
    do { \
        if (!(cond)) { \
            ELOG(msg,p1,p2,p3); \
        } \
    } while (0)
