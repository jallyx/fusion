#pragma once

#include "Thread.h"
#include "AsioHeader.h"

class AsioWorkingThread : public Thread
{
public:
    THREAD_RUNTIME(AsioWorkingThread)

    AsioWorkingThread(boost::asio::io_service &io_service);
    virtual ~AsioWorkingThread();

private:
    virtual void Kernel();

    boost::asio::io_service &io_service_;
};
