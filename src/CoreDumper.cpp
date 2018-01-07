#include "CoreDumper.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Macro.h"

#if defined(__linux__)
    #include <sys/prctl.h>
    #include <sys/resource.h>
    #include <unistd.h>
#endif

CoreDumper::CoreDumper()
: enable_manual_(true)
{
#if defined(__linux__)
    struct rlimit limit;
    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &limit);
    prctl(PR_SET_DUMPABLE, 1);
#endif
}

CoreDumper::~CoreDumper()
{
#if defined(__linux__)
    struct rlimit limit;
    limit.rlim_cur = 0;
    limit.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &limit);
    prctl(PR_SET_DUMPABLE, 0);
#endif
}

void CoreDumper::ManualDump()
{
    static unsigned total = 0;
    if (enable_manual_) {
        char cmdline[256];
        snprintf(cmdline, sizeof(cmdline), "gcore -o core.%x.%x %d",
                 (unsigned)time(nullptr), ++total, getpid());
        system(cmdline);
    }
}
