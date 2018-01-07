#pragma once

#include "Singleton.h"

class CoreDumper : public Singleton<CoreDumper>
{
public:
    CoreDumper();
    virtual ~CoreDumper();

    void ManualDump();

    void EnableManual() { enable_manual_ = true; }
    void DisableManual() { enable_manual_ = false; }

private:
    bool enable_manual_;
};

#define sCoreDumper (*CoreDumper::instance())
