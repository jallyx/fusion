#include "AsyncWorkingThread.h"
#include "AsyncTaskMgr.h"
#include "Exception.h"

AsyncWorkingThread::AsyncWorkingThread(
    ThreadSafeQueue<std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>>> &tasks)
: tasks_(tasks)
{
}

AsyncWorkingThread::~AsyncWorkingThread()
{
}

void AsyncWorkingThread::Kernel()
{
    do {
        std::pair<AsyncTask*, std::weak_ptr<AsyncTaskOwner>> pair;
        while (tasks_.Dequeue(pair)) {
            TRY_BEGIN {
                pair.first->ExecuteInAsync();
            } TRY_END
            CATCH_BEGIN(const IException &e) {
                e.Print();
            } CATCH_END
            CATCH_BEGIN(...) {
            } CATCH_END
            if (!pair.second.expired()) {
                pair.second.lock()->AddTask(pair.first);
            } else {
                delete pair.first;
            }
        }
    } while (sAsyncTaskMgr.WaitTask(&tasks_));
}
