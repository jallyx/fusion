#pragma once

#include "Singleton.h"
#include <fstream>
#include <mutex>

class SignalHandler : public Singleton<SignalHandler>
{
public:
    SignalHandler();
    virtual ~SignalHandler();

private:
    static void InstallSignalHandler(int signum, void(*handler)(int));
    static void UninstallSignalHandler(int signum);

    static void FatalSignalHandler(int signum);
    static void EndSignalHandler(int signum);
    static void CoreSignalHandler(int signum);

    std::ofstream &GrabOutputStream();
    void ReleaseOutputStream();

    std::mutex mutex_;
    std::ofstream ofstream_;
};

#define sSignalHandler (*SignalHandler::instance())
