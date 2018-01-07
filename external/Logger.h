#pragma once

#include "Singleton.h"
#include "Thread.h"
#include "ThreadSafeBlockQueue.h"
#include "ThreadSafePool.h"
#include <stdarg.h>

class Logger : public Thread, public Singleton<Logger>
{
public:
    THREAD_RUNTIME(Logger)

    Logger();
    virtual ~Logger();

    void Log(const char *file, const char *func, int line, char type, const char *fmt, ...);

protected:
    virtual void Kernel();

private:
    std::string *AllocString();
    void FreeString(std::string *s);

    void Color(char type);

    ThreadSafeBlockQueue<std::string*> log_queue_;
    ThreadSafePool<std::string, 256> str_pool_;
};

#define sLogger (*Logger::instance())

#define NLOG(...) sLogger.Log(__FILE__, __FUNCTION__, __LINE__, 'N', __VA_ARGS__)
#define WLOG(...) sLogger.Log(__FILE__, __FUNCTION__, __LINE__, 'W', __VA_ARGS__)
#define ELOG(...) sLogger.Log(__FILE__, __FUNCTION__, __LINE__, 'E', __VA_ARGS__)

#if defined(_DEBUG)
#define DLOG(...) sLogger.Log(__FILE__, __FUNCTION__, __LINE__, 'D', __VA_ARGS__)
#else
#define DLOG(...)
#endif
