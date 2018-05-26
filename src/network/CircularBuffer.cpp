#include "CircularBuffer.h"
#include <string.h>
#include <algorithm>
#include <type_traits>
#include "Debugger.h"

CircularBuffer::CircularBuffer(size_t size)
: base_(new char[size])
, size_(size)
, in_(0)
, out_(0)
{
    assert((size & (size - 1)) == 0);
}

CircularBuffer::~CircularBuffer()
{
    delete[] base_;
}

bool CircularBuffer::IsEmpty() const
{
    return in_ == out_;
}

bool CircularBuffer::IsFull() const
{
    return in_ == out_ + size_;
}

size_t CircularBuffer::GetWritableSpace() const
{
    return out_ + size_ - in_;
}

size_t CircularBuffer::GetReadableSpace() const
{
    return in_ - out_;
}

size_t CircularBuffer::GetSafeDataSize() const
{
    return std::max(int(in_ - out_), 0);
}

size_t CircularBuffer::Write(const char *data, size_t size)
{
    size_t n = 0;
    for (auto i = 0; i < 2 && n < size; ++i) {
        const size_t space = GetContiguiousWritableSpace();
        if (space != 0) {
            const size_t avail = std::min(size - n, space);
            memcpy(GetContiguiousWritableBuffer(), data, avail);
            IncrementContiguiousWritten(avail);
            data += avail, n += avail;
        } else {
            break;
        }
    }
    return n;
}

size_t CircularBuffer::Read(char *data, size_t size)
{
    size_t n = 0;
    for (auto i = 0; i < 2 && n < size; ++i) {
        const size_t space = GetContiguiousReadableSpace();
        if (space != 0) {
            const size_t avail = std::min(size - n, space);
            memcpy(data, GetContiguiousReadableBuffer(), avail);
            IncrementContiguiousRead(avail);
            data += avail, n += avail;
        } else {
            break;
        }
    }
    return n;
}

size_t CircularBuffer::Remove(size_t size)
{
    size_t n = 0;
    for (auto i = 0; i < 2 && n < size; ++i) {
        const size_t space = GetContiguiousReadableSpace();
        if (space != 0) {
            const size_t avail = std::min(size - n, space);
            IncrementContiguiousRead(avail);
            n += avail;
        } else {
            break;
        }
    }
    return n;
}

size_t CircularBuffer::Peek(char *data, size_t size) const
{
    std::aligned_storage<
        sizeof(CircularBuffer), alignof(CircularBuffer)>::type placeholder;
    CircularBuffer &other = *reinterpret_cast<CircularBuffer*>(&placeholder);
    memcpy(&other, this, sizeof(CircularBuffer));
    return other.Read(data, size);
}

size_t CircularBuffer::GetContiguiousWritableSpace() const
{
    return std::min(out_ + size_ - in_, size_ - (in_ & (size_ - 1)));
}

char *CircularBuffer::GetContiguiousWritableBuffer() const
{
    return base_ + (in_ & (size_ - 1));
}

void CircularBuffer::IncrementContiguiousWritten(size_t size)
{
    DBGASSERT(size <= std::min(out_ + size_ - in_, size_ - (in_ & (size_ - 1))));
    in_ += size;
}

size_t CircularBuffer::GetContiguiousReadableSpace() const
{
    return std::min(in_ - out_, size_ - (out_ & (size_ - 1)));
}

const char *CircularBuffer::GetContiguiousReadableBuffer() const
{
    return base_ + (out_ & (size_ - 1));
}

void CircularBuffer::IncrementContiguiousRead(size_t size)
{
    DBGASSERT(size <= std::min(in_ - out_, size_ - (out_ & (size_ - 1))));
    out_ += size;
}
