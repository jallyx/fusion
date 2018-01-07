#include "Exception.h"
#include <stack>
#include "Logger.h"

#if defined(__linux__)
    #include <execinfo.h>
#endif

thread_local std::stack<LongJumpObject> s_long_jump_object_stack;
AutoReleaseLongJumpObject::AutoReleaseLongJumpObject(const char *file, const char *func, int line)
{
    s_long_jump_object_stack.push(LongJumpObject(file, func, line));
}
AutoReleaseLongJumpObject::~AutoReleaseLongJumpObject()
{
    s_long_jump_object_stack.pop();
}

LongJumpObject *LongJumpObject::GetTop()
{
    if (!s_long_jump_object_stack.empty())
        return &s_long_jump_object_stack.top();
    return nullptr;
}
void LongJumpObject::LongJump()
{
    LongJumpObject *obj = GetTop();
    if (obj != nullptr) {
        WLOG("will longjmp to %s, %s (line %d)", obj->file(), obj->func(), obj->line());
        longjmp(obj->GetJumpBuffer(), 1);
    } else {
        WLOG("longjmp not set, will crash!");
    }
}

void LongJumpException::Print() const
{
    WLOG("Already done long jump.");
}

BackTraceException::BackTraceException()
: nptrs_(0)
, backtraces_(nullptr)
{
    TRY_BEGIN {
#if defined(__linux__)
        const int size = 64;
        void *buffer[size];
        nptrs_ = backtrace(buffer, size);
        backtraces_ = backtrace_symbols(buffer, nptrs_);
#endif
    } TRY_END
    CATCH_BEGIN(const IException &e) {
        e.Print();
    } CATCH_END
    CATCH_BEGIN(...) {
    } CATCH_END
}

BackTraceException::~BackTraceException()
{
    if (backtraces_ != nullptr) {
        free(backtraces_);
        backtraces_ = nullptr;
    }
}

void BackTraceException::Print() const
{
    WLOG("%s", what());
    if (backtraces_ != nullptr) {
        for (int index = 0; index < nptrs_; ++index) {
            WLOG("%s", backtraces_[index]);
        }
    }
}
