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

    void SetDumpUpper(unsigned int upper) { dump_upper_ = upper; }

private:
    bool enable_manual_;

    unsigned int dump_upper_;
    unsigned int dump_count_;
};

#define sCoreDumper (*CoreDumper::instance())
