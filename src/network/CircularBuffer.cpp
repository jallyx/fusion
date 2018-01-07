#include "CircularBuffer.h"
#include <string.h>
#include <algorithm>
#include "Debugger.h"

CircularBuffer::CircularBuffer(size_t size)
: base_(new char[size + 1])
, size_(size + 1)
{
    outptr_ = inptr_ = base_;
}

CircularBuffer::~CircularBuffer()
{
    delete [] base_;
}

void CircularBuffer::Reset()
{
    outptr_ = inptr_ = base_;
}

bool CircularBuffer::IsEmpty() const
{
    return outptr_ == inptr_;
}

bool CircularBuffer::IsFull() const
{
    return outptr_ == inptr_ + 1 || outptr_ + size_ == inptr_ + 1;
}

size_t CircularBuffer::GetWritableSpace() const
{
    return size_ - GetReadableSpace() - 1;
}

size_t CircularBuffer::GetReadableSpace() const
{
    if (inptr_ >= outptr_)
        return inptr_ - outptr_;
    return size_ + inptr_ - outptr_;
}

size_t CircularBuffer::Write(const char *data, size_t size)
{
    size_t offset = 0;
    for (auto i = 0; i < 2 && offset < size; ++i) {
        const size_t space = GetContiguiousWritableSpace();
        if (space != 0) {
            const size_t length = std::min(size - offset, space);
            memcpy(GetContiguiousWritableBuffer(), data + offset, length);
            IncrementContiguiousWritten(length);
            offset += length;
        } else {
            break;
        }
    }
    return offset;
}

size_t CircularBuffer::Read(char *data, size_t size)
{
    size_t offset = 0;
    for (auto i = 0; i < 2 && offset < size; ++i) {
        const size_t space = GetContiguiousReadableSpace();
        if (space != 0) {
            const size_t length = std::min(size - offset, space);
            memcpy(data + offset, GetContiguiousReadableBuffer(), length);
            IncrementContiguiousRead(length);
            offset += length;
        } else {
            break;
        }
    }
    return offset;
}

size_t CircularBuffer::Remove(size_t size)
{
    size_t offset = 0;
    for (auto i = 0; i < 2 && offset < size; ++i) {
        const size_t space = GetContiguiousReadableSpace();
        if (space != 0) {
            const size_t length = std::min(size - offset, space);
            IncrementContiguiousRead(length);
            offset += length;
        } else {
            break;
        }
    }
    return offset;
}

size_t CircularBuffer::Peek(char *data, size_t size) const
{
    char placeholder[sizeof(CircularBuffer)];
    memcpy(placeholder, this, sizeof(CircularBuffer));
    CircularBuffer &other = *reinterpret_cast<CircularBuffer*>(placeholder);
    return other.Read(data, size);
}

size_t CircularBuffer::GetContiguiousWritableSpace() const
{
    if (inptr_ >= outptr_)
        return base_ + size_ - inptr_ - (base_ == outptr_ ? 1 : 0);
    return outptr_ - inptr_ - 1;
}

char *CircularBuffer::GetContiguiousWritableBuffer() const
{
    return inptr_;
}

void CircularBuffer::IncrementContiguiousWritten(size_t size)
{
    if (inptr_ >= outptr_) {
        DBGASSERT(inptr_ + size + (base_ == outptr_ ? 1 : 0) <= base_ + size_);
        if (inptr_ + size < base_ + size_) {
            inptr_ += size;
        } else {
            inptr_ = base_;
        }
    } else {
        DBGASSERT(inptr_ + size + 1 <= outptr_);
        inptr_ += size;
    }
}

size_t CircularBuffer::GetContiguiousReadableSpace() const
{
    if (outptr_ <= inptr_)
        return inptr_ - outptr_;
    return base_ + size_ - outptr_;
}

const char *CircularBuffer::GetContiguiousReadableBuffer() const
{
    return outptr_;
}

void CircularBuffer::IncrementContiguiousRead(size_t size)
{
    if (outptr_ <= inptr_) {
        DBGASSERT(outptr_ + size <= inptr_);
        if (outptr_ + size < inptr_) {
            outptr_ += size;
        } else {
            outptr_ = inptr_ = base_;
        }
    } else {
        DBGASSERT(outptr_ + size <= base_ + size_);
        if (outptr_ + size < base_ + size_) {
            outptr_ += size;
        } else {
            outptr_ = base_;
        }
    }
}
