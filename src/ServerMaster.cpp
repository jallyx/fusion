#include "ServerMaster.h"
#include "network/Connection.h"
#include "NetPacket.h"
#include "Logger.h"
#include "OS.h"
#include "CoreDumper.h"
#include "SignalHandler.h"
#include "System.h"

IServerMaster *IServerMaster::instance_ = nullptr;
IServerMaster::IServerMaster()
: is_end_(false)
, delay_end_(0)
{
    assert(instance_ == nullptr);
    instance_ = this;

    System::Init();
    System::Update();

    Connection::InitSendBufferPool();
    INetPacket::InitNetPacketPool();
    CoreDumper::newInstance();
    SignalHandler::newInstance();
    Logger::newInstance();
}

IServerMaster::~IServerMaster()
{
    instance_ = nullptr;

    Connection::ClearSendBufferPool();
    INetPacket::ClearNetPacketPool();
    CoreDumper::deleteInstance();
    SignalHandler::deleteInstance();
    Logger::deleteInstance();
}

bool IServerMaster::ParseConfigFile(KeyFile &config, const std::string &file)
{
    if (config.ParseFile(file.c_str())) {
        return true;
    }
    std::string path(OS::GetProgramDirectory());
    path.append(1, os::sep).append(file);
    if (config.ParseFile(path.c_str())) {
        return true;
    }
    return false;
}

int IServerMaster::Initialize(int argc, char *argv[])
{
    const std::string configFile = GetConfigFile();
    if (!ParseConfigFile(config_, configFile)) {
        printf("open config file '%s' failed.\n", configFile.c_str());
        return -1;
    }

    if (!sLogger.Start()) {
        printf("sLogger.Start() failed.\n");
        return -1;
    }

    if (!InitDBPool()) {
        ELOG("InitDBPool() failed.");
        return -1;
    }
    if (!LoadDBData()) {
        ELOG("LoadDBData() failed.");
        return -1;
    }

    return 0;
}

int IServerMaster::Run(int argc, char *argv[])
{
    if (!StartServices()) {
        ELOG("StartService() failed.");
        return -1;
    }

    auto handler = [=](int c) {
        if (c == 'q' || c == 'Q') {
            End(0);
        }
    };

    while (!is_end_ || delay_end_ > 0) {
        OS::SleepMS(1000);
        Tick();

#if defined(_WIN32)
        HANDLE keyboard = GetStdHandle(STD_INPUT_HANDLE);
        while (WaitForSingleObject(keyboard, 0) == WAIT_OBJECT_0) {
            INPUT_RECORD e;
            DWORD n;
            if (ReadConsoleInput(keyboard, &e, 1, &n)) {
                if (n == 1 && e.EventType == KEY_EVENT) {
                    int c = e.Event.KeyEvent.uChar.AsciiChar;
                    handler(c);
                }
            }
        }
#else
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(STDIN_FILENO, &rset);
        struct timeval tv = {0, 0};
        if (select(STDIN_FILENO + 1, &rset, nullptr, nullptr, &tv) == 1) {
            int c = getchar();
            handler(c);
        }
#endif

        delay_end_ = std::max(0, delay_end_ - 1000);
    }

    StopServices();
    sLogger.Stop();

    return 0;
}

void IServerMaster::End(int delay)
{
    delay_end_ = delay;
    is_end_ = true;
}
