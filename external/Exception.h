#pragma once

#include <setjmp.h>
#include <exception>

#define THROW_EXCEPTION(e) (throw e)

#define TRY_BEGIN \
    AutoReleaseLongJumpObject _(__FILE__, __FUNCTION__, __LINE__); \
    try { \
        if (setjmp(LongJumpObject::GetTop()->GetJumpBuffer()) != 0) { \
            THROW_EXCEPTION(LongJumpException()); \
        }
#define TRY_END \
    }
#define CATCH_BEGIN(e) \
    catch (e) {
#define CATCH_END \
    }

class AutoReleaseLongJumpObject
{
public:
    AutoReleaseLongJumpObject(const char *file, const char *func, int line);
    ~AutoReleaseLongJumpObject();
};

class LongJumpObject
{
public:
    LongJumpObject(const char *file, const char *func, int line)
        : file_(file), func_(func), line_(line)
    {}

    jmp_buf &GetJumpBuffer() { return jmp_buf_; }

    const char *file() const { return file_; }
    const char *func() const { return func_; }
    int line() const { return line_; }

    static LongJumpObject *GetTop();
    static void LongJump();

private:
    jmp_buf jmp_buf_;
    const char *file_;
    const char *func_;
    int line_;
};

class IException : public std::exception
{
public:
    IException() = default;
    virtual ~IException() = default;

    virtual void Print() const = 0;
};

class LongJumpException : public IException
{
public:
    virtual void Print() const;
};

class BackTraceException : public IException
{
public:
    BackTraceException();
    virtual ~BackTraceException();

    virtual void Print() const;

protected:
    int nptrs() const { return nptrs_; }
    char * const *backtraces() const { return backtraces_; }

private:
    int nptrs_;
    char **backtraces_;
};

class InternalException : public BackTraceException
{
public:
    virtual const char *what() const noexcept {
        return "InternalException";
    }
};

class NetStreamException : public BackTraceException
{
public:
    virtual const char *what() const noexcept {
        return "NetStreamException";
    }
};

class RecvDataException : public NetStreamException
{
public:
    virtual const char *what() const noexcept {
        return "RecvDataException";
    }
};

class SendDataException : public NetStreamException
{
public:
    virtual const char *what() const noexcept {
        return "SendDataException";
    }
};
