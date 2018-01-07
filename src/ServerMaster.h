#pragma once

#include "KeyFile.h"
#include "noncopyable.h"

class IServerMaster : public noncopyable
{
public:
    IServerMaster();
    virtual ~IServerMaster();

    virtual bool InitSingleton() = 0;
    virtual void FinishSingleton() = 0;

    int Initialize(int argc, char *argv[]);
    int Run(int argc, char *argv[]);
    void End(int delay);

    bool IsEnd() const { return is_end_; }
    const KeyFile &GetConfig() const { return config_; }
    static IServerMaster &GetInstance() { return *instance_; }

protected:
    virtual bool InitDBPool() = 0;
    virtual bool LoadDBData() = 0;

    virtual bool StartServices() = 0;
    virtual void StopServices() = 0;

    virtual void Tick() = 0;

    virtual std::string GetConfigFile() = 0;

private:
    KeyFile config_;

    bool is_end_;
    int delay_end_;

    static IServerMaster *instance_;
};
