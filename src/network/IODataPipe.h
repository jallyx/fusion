#pragma once

#include "SendBuffer.h"
#include "CircularBuffer.h"
#include "zlib/ZlibStream.h"
#include "lz4/Lz4Stream.h"

class ISendDataPipe
{
public:
    ISendDataPipe() : active_(nullptr), prev_(nullptr) {}
    virtual ~ISendDataPipe() { delete prev_; }
    virtual const char *GetSendDataBuffer(size_t &size) = 0;
    virtual void RemoveSendData(size_t size) = 0;
    virtual bool HasSendDataAwaiting() const = 0;
    virtual size_t GetSendDataSize() const = 0;
    bool IsActive() const { return *active_; }
    void Init(ISendDataPipe *prev) {
        prev_ = prev, active_ = prev->active_;
    }
protected:
    const bool *active_;
    ISendDataPipe *prev_;
};

class IRecvDataPipe
{
public:
    IRecvDataPipe() : active_(nullptr), next_(nullptr) {}
    virtual ~IRecvDataPipe() { delete next_; }
    virtual char *GetRecvDataBuffer(size_t &size) = 0;
    virtual void IncrementRecvData(size_t size) = 0;
    bool IsActive() const { return *active_; }
    void Init(IRecvDataPipe *next) {
        next_ = next, active_ = next->active_;
    }
protected:
    const bool *active_;
    IRecvDataPipe *next_;
};


class SendDataFirstPipe : public ISendDataPipe
{
public:
    SendDataFirstPipe(const bool &active);
    virtual const char *GetSendDataBuffer(size_t &size);
    virtual void RemoveSendData(size_t size);
    virtual bool HasSendDataAwaiting() const;
    virtual size_t GetSendDataSize() const;
    SendBuffer &GetBuffer() { return buffer_; }
private:
    SendBuffer buffer_;
};

class RecvDataLastPipe : public IRecvDataPipe
{
public:
    RecvDataLastPipe(
        std::function<void(INetPacket*)> &&receiver, const bool &active);
    virtual char *GetRecvDataBuffer(size_t &size);
    virtual void IncrementRecvData(size_t size);
private:
    INetPacket *ReadPacketFromBuffer();
    const std::function<void(INetPacket*)> receiver_;
    CircularBuffer buffer_;
};


class SendDataZlibPipe : public ISendDataPipe
{
public:
    SendDataZlibPipe();
    virtual const char *GetSendDataBuffer(size_t &size);
    virtual void RemoveSendData(size_t size);
    virtual bool HasSendDataAwaiting() const;
    virtual size_t GetSendDataSize() const;
private:
    void Compress();
    zlib::DeflateStream compress_;
    CircularBuffer buffer_;
    bool flush_;
};

class RecvDataZlibPipe : public IRecvDataPipe
{
public:
    RecvDataZlibPipe();
    virtual char *GetRecvDataBuffer(size_t &size);
    virtual void IncrementRecvData(size_t size);
private:
    void Decompress();
    zlib::InflateStream decompress_;
    CircularBuffer buffer_;
};


class SendDataLz4Pipe : public ISendDataPipe
{
public:
    SendDataLz4Pipe();
    virtual const char *GetSendDataBuffer(size_t &size);
    virtual void RemoveSendData(size_t size);
    virtual bool HasSendDataAwaiting() const;
    virtual size_t GetSendDataSize() const;
private:
    void Compress();
    lz4::CompressStream compress_;
    CircularBuffer buffer_;
    bool flush_;
};

class RecvDataLz4Pipe : public IRecvDataPipe
{
public:
    RecvDataLz4Pipe();
    virtual char *GetRecvDataBuffer(size_t &size);
    virtual void IncrementRecvData(size_t size);
private:
    void Decompress();
    lz4::DecompressStream decompress_;
    CircularBuffer buffer_;
};
