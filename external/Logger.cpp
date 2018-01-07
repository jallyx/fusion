#include "Logger.h"
#include <string.h>
#include "Macro.h"
#include "System.h"

enum LogType {
    LogNormal,
    LogWarning,
    LogError,
    LogDebug,
    LogEnd,
};

#if defined(_WIN32)
static WORD color[] = {
    FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
    FOREGROUND_RED | FOREGROUND_INTENSITY,
    FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
    FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,
};
#else
static const char *color[] = {
    "\033[22;32m",  // green
    "\033[01;33m",  // yellow
    "\033[22;31m",  // red
    "\033[22;37m",  // gray
    "\033[0m",  // end
};
#endif

Logger::Logger()
{
}

Logger::~Logger()
{
    std::string *s = nullptr;
    while (log_queue_.Dequeue(s, 0)) {
        FreeString(s);
    }
    while ((s = str_pool_.Get()) != nullptr) {
        delete s;
    }
}

void Logger::Log(const char *file, const char *func, int line, char type, const char *fmt, ...)
{
    char strtime[(4+1+2+1+2 + 1 + 2+1+2+1+2) + 1];
    const struct tm tm = GET_DATE_TIME;
    strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", &tm);

    va_list args;
    va_start(args, fmt);
    char strlog[4096];
    vsnprintf(strlog, sizeof(strlog), fmt, args);
    va_end(args);

    char strpos[512];
    if (type == 'E') {
        const char *filename = strrchr(file, os::sep) + 1;
        snprintf(strpos, sizeof(strpos), " (FILE:%s FUNC:%s LINE:%d)", filename, func, line);
    } else {
        strpos[0] = '\0';
    }

    std::string *s = AllocString();
    (*s).reserve(1+1+strlen(strtime)+1+strlen(strlog)+strlen(strpos));
    (*s).append(1,type).append(1,'|').append(strtime).append(1,'|').append(strlog).append(strpos);

    log_queue_.Enqueue(s);
}

void Logger::Kernel()
{
    std::string *s = nullptr;
    while (log_queue_.Dequeue(s, 100)) {
        Color(s->at(0));
        printf("%s\n", s->c_str());
        Color('\0');
        FreeString(s);
    }
    fflush(stdout);
}

void Logger::Color(char type)
{
    LogType log_type = LogEnd;
    switch (type) {
    case 'N': log_type = LogNormal; break;
    case 'W': log_type = LogWarning; break;
    case 'E': log_type = LogError; break;
    case 'D': log_type = LogDebug; break;
    }
#if defined(_WIN32)
    static HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hStdOut, color[log_type]);
#else
    printf("%s", color[log_type]);
#endif
}

std::string *Logger::AllocString()
{
    std::string *s = str_pool_.Get();
    if (s != nullptr) {
        s->clear();
        return s;
    }
    return new std::string();
}

void Logger::FreeString(std::string *s)
{
    if (!str_pool_.Put(s)) {
        delete s;
    }
}
