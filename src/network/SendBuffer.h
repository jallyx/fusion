#pragma once

#include <queue>
#include "NetBuffer.h"
#include "NetPacket.h"
#include "Session.h"

// be useful for:
// multi producer, single consumer.
// be careful about:
// io thread maybe is using the out buffer at any time.
// we must guarantee the out buffer is addressable.

class Connection::SendBuffer : public Session::ISendBuffer
{
public:
    SendBuffer()
        : in_buffer_(nullptr)
        , out_buffer_(nullptr)
    {}
    virtual ~SendBuffer() {
        ClearData();
        FreeBuffer(in_buffer_);
        FreeBuffer(out_buffer_);
    }

    bool IsEmpty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (out_buffer_ && !out_buffer_->IsReadableEmpty())
            return false;
        if (in_buffer_ && !in_buffer_->IsReadableEmpty())
            return false;
        return in_buffers_.empty();
    }

    void ClearData() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (out_buffer_ != nullptr)
            out_buffer_->Clear();
        if (in_buffer_ != nullptr)
            in_buffer_->Clear();
        for (; !in_buffers_.empty(); in_buffers_.pop()) {
            FreeBuffer(in_buffers_.front());
        }
    }

    const char *GetSendDataBuffer(size_t &size) {
        if (out_buffer_ && !out_buffer_->IsReadableEmpty()) {
            size = out_buffer_->GetReadableSize();
            return out_buffer_->GetReadableBuffer();
        }
        do {
            std::lock_guard<std::mutex> lock(mutex_);
            if (in_buffers_.empty()) {
                std::swap(out_buffer_, in_buffer_);
                if (in_buffer_ != nullptr)
                    in_buffer_->Clear();
            } else {
                if (out_buffer_ != nullptr)
                    FreeBuffer(out_buffer_);
                out_buffer_ = in_buffers_.front();
                in_buffers_.pop();
            }
        } while (0);
        if (out_buffer_ && !out_buffer_->IsReadableEmpty()) {
            size = out_buffer_->GetReadableSize();
            return out_buffer_->GetReadableBuffer();
        }
        return nullptr;
    }
    void RemoveSendData(size_t size) {
        if (out_buffer_ != nullptr) {
            out_buffer_->AdjustReadPos(size);
            if (out_buffer_->IsReadableEmpty())
                out_buffer_->SkipReadPosToEnd();
        }
    }

    virtual void WritePacket(const INetPacket &pck) {
        std::lock_guard<std::mutex> lock(mutex_);
        Header(pck.GetOpcode(), pck.GetReadableSize() + INetPacket::Header::SIZE);
        Append(pck.GetReadableBuffer(), pck.GetReadableSize());
    }
    virtual void WritePacket(const char *data, size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        Append(data, size);
    }
    virtual void WritePacket(const INetPacket &pck, const INetPacket &data) {
        std::lock_guard<std::mutex> lock(mutex_);
        Header(pck.GetOpcode(), pck.GetReadableSize() + INetPacket::Header::SIZE +
                                data.GetReadableSize() + INetPacket::Header::SIZE);
        Append(pck.GetReadableBuffer(), pck.GetReadableSize());
        Header(data.GetOpcode(), data.GetReadableSize() + INetPacket::Header::SIZE);
        Append(data.GetReadableBuffer(), data.GetReadableSize());
    }
    virtual void WritePacket(const INetPacket &pck, const char *data, size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        Header(pck.GetOpcode(), pck.GetReadableSize() + INetPacket::Header::SIZE + size);
        Append(pck.GetReadableBuffer(), pck.GetReadableSize());
        Append(data, size);
    }

private:
    void Header(uint32 cmd, size_t len) {
        TNetPacket<INetPacket::Header::SIZE> wrapper;
        wrapper.WriteHeader(INetPacket::Header(cmd, len));
        Append(wrapper.GetReadableBuffer(), wrapper.GetReadableSize());
    }

    void Append(const char *data, size_t size) {
        size_t offset = 0;
        while (offset < size) {
            if (in_buffer_ == nullptr) {
                in_buffer_ = AllocBuffer();
            }
            size_t space = in_buffer_->GetWritableSize();
            if (space != 0) {
                size_t length = std::min(size - offset, space);
                in_buffer_->Append(data + offset, length);
                offset += length;
            } else {
                in_buffers_.push(in_buffer_);
                in_buffer_ = nullptr;
            }
        }
    }

    typedef TNetBuffer<65536> DataBuffer;
    std::queue<DataBuffer*> in_buffers_;
    DataBuffer *in_buffer_;
    DataBuffer *out_buffer_;
    mutable std::mutex mutex_;

public:
    static void InitBufferPool() {
    }
    static void ClearBufferPool() {
        DataBuffer *buffer = nullptr;
        while ((buffer = buffer_pool_.Get()) != nullptr) {
            delete buffer;
        }
    }

private:
    static DataBuffer *AllocBuffer() {
        DataBuffer *buffer = nullptr;
        if ((buffer = buffer_pool_.Get()) != nullptr) {
            buffer->Clear();
            return buffer;
        }
        return new DataBuffer();
    }
    static void FreeBuffer(DataBuffer *buffer) {
        if (!buffer_pool_.Put(buffer)) {
            delete buffer;
        }
    }

    static ThreadSafePool<DataBuffer, 128> buffer_pool_;
};

ThreadSafePool<Connection::SendBuffer::DataBuffer, 128>
    Connection::SendBuffer::buffer_pool_;
