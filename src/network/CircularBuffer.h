#pragma once

#include <stddef.h>
#include "noncopyable.h"

// be useful for:
// single producer, single consumer.

class CircularBuffer : public noncopyable
{
public:
    CircularBuffer(size_t size);
    ~CircularBuffer();

    bool IsEmpty() const;
    bool IsFull() const;

    size_t GetWritableSpace() const;
    size_t GetReadableSpace() const;

    size_t Write(const char *data, size_t size);
    size_t Read(char *data, size_t size);

    size_t Remove(size_t size);
    size_t Peek(char *data, size_t size) const;

    size_t GetContiguiousWritableSpace() const;
    char *GetContiguiousWritableBuffer() const;
    void IncrementContiguiousWritten(size_t size);

    size_t GetContiguiousReadableSpace() const;
    const char *GetContiguiousReadableBuffer() const;
    void IncrementContiguiousRead(size_t size);

private:
    char * const base_;
    size_t const size_;

    size_t in_;
    size_t out_;
};
