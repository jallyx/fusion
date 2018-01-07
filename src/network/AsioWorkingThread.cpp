#include "AsioWorkingThread.h"

AsioWorkingThread::AsioWorkingThread(asio::io_service &io_service)
: io_service_(io_service)
{
}

AsioWorkingThread::~AsioWorkingThread()
{
}

void AsioWorkingThread::Kernel()
{
    io_service_.run();
}
