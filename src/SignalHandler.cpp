#include "SignalHandler.h"
#include <signal.h>
#include <string.h>
#include "CoreDumper.h"
#include "Exception.h"
#include "Logger.h"
#include "OS.h"
#include "ServerMaster.h"
#include "System.h"

#if defined(__linux__)
    #include <unistd.h>
#endif

class SignalException : public BackTraceException
{
public:
    SignalException(int signum)
        : signum_(signum)
    {}

    virtual const char *what() const noexcept {
        return "SignalException";
    }

    void WriteStream(std::ostream &stream) const {
        int nptr = nptrs();
        char * const *strings = backtraces();
        struct tm tm = GET_DATE_TIME;
        char strtime[(4+1+2+1+2 + 1 + 2+1+2+1+2) + 1];
        strftime(strtime, sizeof(strtime), "%Y-%m-%d %H:%M:%S", &tm);
        stream << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n";
        stream << strtime << ": signum = " << signum_ << ", nptrs = " << nptr << "\n";
        if (nptr != 0 && strings != nullptr) {
            for (int index = 0; index < nptr; ++index) {
                stream << strings[index] << "\n";
            }
        }
        stream << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n";
    }

private:
    int signum_;
};

SignalHandler::SignalHandler()
{
    InstallSignalHandler(SIGSEGV, SignalHandler::FatalSignalHandler);
    InstallSignalHandler(SIGILL, SignalHandler::FatalSignalHandler);
    InstallSignalHandler(SIGABRT, SignalHandler::FatalSignalHandler);
    InstallSignalHandler(SIGFPE, SignalHandler::FatalSignalHandler);
#if defined(__linux__)
    InstallSignalHandler(SIGBUS, SignalHandler::FatalSignalHandler);
    InstallSignalHandler(SIGPIPE, SignalHandler::FatalSignalHandler);
    InstallSignalHandler(SIGUSR1, SignalHandler::EndSignalHandler);
    InstallSignalHandler(SIGUSR2, SignalHandler::CoreSignalHandler);
#endif
}

SignalHandler::~SignalHandler()
{
    UninstallSignalHandler(SIGSEGV);
    UninstallSignalHandler(SIGILL);
    UninstallSignalHandler(SIGABRT);
    UninstallSignalHandler(SIGFPE);
#if defined(__linux__)
    UninstallSignalHandler(SIGBUS);
    UninstallSignalHandler(SIGPIPE);
    UninstallSignalHandler(SIGUSR1);
    UninstallSignalHandler(SIGUSR2);
#endif
}

void SignalHandler::InstallSignalHandler(int signum, void(*handler)(int))
{
#if defined(__linux__)
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    act.sa_flags = SA_NODEFER | SA_RESTART;
    sigaction(signum, &act, nullptr);
#endif
}

void SignalHandler::UninstallSignalHandler(int signum)
{
#if defined(__linux__)
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;
    sigaction(signum, &act, nullptr);
#endif
}

void SignalHandler::FatalSignalHandler(int signum)
{
    sCoreDumper.ManualDump();
    ELOG("fatal signal captured, signum = %d.", signum);
    SignalException se(signum);
    se.Print();
    std::ofstream &stream = sSignalHandler.GrabOutputStream();
    if (stream.is_open()) {
        se.WriteStream(stream);
    }
    sSignalHandler.ReleaseOutputStream();
    LongJumpObject::LongJump();
}

void SignalHandler::EndSignalHandler(int signum)
{
    IServerMaster::GetInstance().End(0);
}

void SignalHandler::CoreSignalHandler(int signum)
{
    sCoreDumper.DisableManual();
}

std::ofstream &SignalHandler::GrabOutputStream()
{
    mutex_.lock();
    if (!ofstream_.is_open()) {
        struct tm tm = GET_DATE_TIME;
        char strtime[4+2+2+2+2+2 + 1];
        strftime(strtime, sizeof(strtime), "%Y%m%d%H%M%S", &tm);
        char filename[256];
        snprintf(filename, sizeof(filename), "%s_%s(%d)_exceptions.log",
                 strtime, OS::GetProgramName().c_str(), getpid());
        ofstream_.open(filename, std::ios::app|std::ios::ate);
    }
    return ofstream_;
}

void SignalHandler::ReleaseOutputStream()
{
    ofstream_.flush();
    mutex_.unlock();
}
