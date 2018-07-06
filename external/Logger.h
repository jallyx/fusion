#pragma once

#include "Singleton.h"
#include "Thread.h"
#include "ThreadSafeBlockQueue.h"
#include "Exception.h"

class Logger : public Thread, public Singleton<Logger>
{
public:
    THREAD_RUNTIME(Logger)

    Logger();
    virtual ~Logger();

    struct LogInfo {
        const char *file;
        const char *func;
        int line;
        std::string s;
    };

    void Log(const LogInfo &info, char type, const char *fmt, ...);

protected:
    virtual void Kernel();

private:
    std::string *AllocString();
    void FreeString(std::string *s);

    void Color(char type);

    ThreadSafeBlockQueue<std::string*, 256> log_queue_;
    ThreadSafePool<std::string, 256> str_pool_;
};

#define sLogger (*Logger::instance())

#define sWLogInfo {__FILE__, __FUNCTION__, __LINE__}
#define sELogInfo {__FILE__, __FUNCTION__, __LINE__, sBackTrace}

#define NLOG(...) sLogger.Log({}, 'N', __VA_ARGS__)
#define WLOG(...) sLogger.Log(sWLogInfo, 'W', __VA_ARGS__)
#define ELOG(...) sLogger.Log(sELogInfo, 'E', __VA_ARGS__)

#if defined(_DEBUG)
#define DLOG(...) sLogger.Log({}, 'D', __VA_ARGS__)
#else
#define DLOG(...)
#endif
