#include "IODataPipe.h"

SendDataFirstPipe::SendDataFirstPipe(const bool &active)
{
    active_ = &active;
}

const char *SendDataFirstPipe::GetSendDataBuffer(size_t &size)
{
    return buffer_.GetSendDataBuffer(size);
}

void SendDataFirstPipe::RemoveSendData(size_t size)
{
    buffer_.RemoveSendData(size);
}

bool SendDataFirstPipe::HasSendDataAwaiting() const
{
    return buffer_.HasSendDataAvail();
}

size_t SendDataFirstPipe::GetSendDataSize() const
{
    return buffer_.GetDataSize();
}


RecvDataLastPipe::RecvDataLastPipe(
    std::function<void(INetPacket*)> &&receiver, const bool &active)
: receiver_(std::move(receiver))
, buffer_(MAX_NET_PACKET_SIZE + 1)
{
    active_ = &active;
}

char *RecvDataLastPipe::GetRecvDataBuffer(size_t &size)
{
    size = buffer_.GetContiguiousWritableSpace();
    return buffer_.GetContiguiousWritableBuffer();
}

void RecvDataLastPipe::IncrementRecvData(size_t size)
{
    buffer_.IncrementContiguiousWritten(size);
    while (IsActive()) {
        INetPacket *pck = ReadPacketFromBuffer();
        if (pck != nullptr) {
            receiver_(pck);
        } else {
            break;
        }
    }
}

INetPacket *RecvDataLastPipe::ReadPacketFromBuffer()
{
    if (buffer_.GetReadableSpace() < INetPacket::Header::SIZE) {
        return nullptr;
    }

    INetPacket::Header header;
    TNetPacket<INetPacket::Header::SIZE> wrapper;
    wrapper.Erlarge(INetPacket::Header::SIZE);
    buffer_.Peek((char*)wrapper.GetBuffer(), wrapper.GetTotalSize());
    wrapper.ReadHeader(header);
    if (buffer_.GetReadableSpace() < header.len) {
        return nullptr;
    }

    size_t size = header.len - INetPacket::Header::SIZE;
    INetPacket *pck = INetPacket::New(header.cmd, size);
    pck->Erlarge(size);
    buffer_.Remove(INetPacket::Header::SIZE);
    buffer_.Read((char*)pck->GetBuffer(), pck->GetTotalSize());
    return pck;
}


SendDataZlibPipe::SendDataZlibPipe()
: buffer_(1 << 12)
, flush_(true)
{
}

const char *SendDataZlibPipe::GetSendDataBuffer(size_t &size)
{
    Compress();
    size = buffer_.GetContiguiousReadableSpace();
    return buffer_.GetContiguiousReadableBuffer();
}

void SendDataZlibPipe::RemoveSendData(size_t size)
{
    buffer_.IncrementContiguiousRead(size);
}

bool SendDataZlibPipe::HasSendDataAwaiting() const
{
    return !buffer_.IsEmpty() || prev_->HasSendDataAwaiting();
}

size_t SendDataZlibPipe::GetSendDataSize() const
{
    return buffer_.GetSafeDataSize() + prev_->GetSendDataSize();
}

void SendDataZlibPipe::Compress()
{
    while (IsActive() && !buffer_.IsFull()) {
        size_t inlen = 0;
        const char *in = prev_->GetSendDataBuffer(inlen);
        if (in != nullptr && inlen != 0) {
            size_t outlen = buffer_.GetContiguiousWritableSpace();
            char *out = buffer_.GetContiguiousWritableBuffer();
            if (compress_.Deflate(in, inlen, out, outlen)) {
                if (inlen != 0) {
                    prev_->RemoveSendData(inlen);
                }
                if (outlen != 0) {
                    buffer_.IncrementContiguiousWritten(outlen);
                }
                flush_ = false;
            } else {
                THROW_EXCEPTION(SendDataException());
            }
        } else {
            break;
        }
    }

    while (!flush_ && IsActive() && !buffer_.IsFull()) {
        size_t availen = buffer_.GetContiguiousWritableSpace();
        char *out = buffer_.GetContiguiousWritableBuffer();
        auto outlen = availen;
        if (compress_.Flush(out, outlen)) {
            if (outlen != 0) {
                buffer_.IncrementContiguiousWritten(outlen);
            }
            if (outlen < availen) {
                flush_ = true;
            }
        } else {
            THROW_EXCEPTION(SendDataException());
        }
    }
}


RecvDataZlibPipe::RecvDataZlibPipe()
: buffer_(1 << 12)
{
}

char *RecvDataZlibPipe::GetRecvDataBuffer(size_t &size)
{
    size = buffer_.GetContiguiousWritableSpace();
    return buffer_.GetContiguiousWritableBuffer();
}

void RecvDataZlibPipe::IncrementRecvData(size_t size)
{
    buffer_.IncrementContiguiousWritten(size);
    Decompress();
}

void RecvDataZlibPipe::Decompress()
{
    while (IsActive() && !buffer_.IsEmpty()) {
        size_t outlen = 0;
        char *out = next_->GetRecvDataBuffer(outlen);
        if (out != nullptr && outlen != 0) {
            size_t inlen = buffer_.GetContiguiousReadableSpace();
            const char *in = buffer_.GetContiguiousReadableBuffer();
            if (decompress_.Inflate(in, inlen, out, outlen)) {
                if (inlen != 0) {
                    buffer_.IncrementContiguiousRead(inlen);
                }
                if (outlen != 0) {
                    next_->IncrementRecvData(outlen);
                }
            } else {
                THROW_EXCEPTION(RecvDataException());
            }
        }
    }
}

SendDataLz4Pipe::SendDataLz4Pipe()
: buffer_(1 << 12)
, flush_(true)
{
}

const char *SendDataLz4Pipe::GetSendDataBuffer(size_t &size)
{
    Compress();
    size = buffer_.GetContiguiousReadableSpace();
    return buffer_.GetContiguiousReadableBuffer();
}

void SendDataLz4Pipe::RemoveSendData(size_t size)
{
    buffer_.IncrementContiguiousRead(size);
}

bool SendDataLz4Pipe::HasSendDataAwaiting() const
{
    return !buffer_.IsEmpty() || prev_->HasSendDataAwaiting();
}

size_t SendDataLz4Pipe::GetSendDataSize() const
{
    return buffer_.GetSafeDataSize() + prev_->GetSendDataSize();
}

void SendDataLz4Pipe::Compress()
{
    while (IsActive() && !buffer_.IsFull()) {
        size_t inlen = 0;
        const char *in = prev_->GetSendDataBuffer(inlen);
        if (in != nullptr && inlen != 0) {
            size_t outlen = buffer_.GetContiguiousWritableSpace();
            char *out = buffer_.GetContiguiousWritableBuffer();
            if (compress_.Compress(in, inlen, out, outlen)) {
                if (inlen != 0) {
                    prev_->RemoveSendData(inlen);
                }
                if (outlen != 0) {
                    buffer_.IncrementContiguiousWritten(outlen);
                }
                flush_ = false;
            } else {
                THROW_EXCEPTION(SendDataException());
            }
        } else {
            break;
        }
    }

    while (!flush_ && IsActive() && !buffer_.IsFull()) {
        size_t availen = buffer_.GetContiguiousWritableSpace();
        char *out = buffer_.GetContiguiousWritableBuffer();
        auto outlen = availen;
        if (compress_.Flush(out, outlen)) {
            if (outlen != 0) {
                buffer_.IncrementContiguiousWritten(outlen);
            }
            if (outlen < availen) {
                flush_ = true;
            }
        } else {
            THROW_EXCEPTION(SendDataException());
        }
    }
}


RecvDataLz4Pipe::RecvDataLz4Pipe()
: buffer_(1 << 12)
{
}

char *RecvDataLz4Pipe::GetRecvDataBuffer(size_t &size)
{
    size = buffer_.GetContiguiousWritableSpace();
    return buffer_.GetContiguiousWritableBuffer();
}

void RecvDataLz4Pipe::IncrementRecvData(size_t size)
{
    buffer_.IncrementContiguiousWritten(size);
    Decompress();
}

void RecvDataLz4Pipe::Decompress()
{
    while (IsActive() && !buffer_.IsEmpty()) {
        size_t outlen = 0;
        char *out = next_->GetRecvDataBuffer(outlen);
        if (out != nullptr && outlen != 0) {
            size_t inlen = buffer_.GetContiguiousReadableSpace();
            const char *in = buffer_.GetContiguiousReadableBuffer();
            if (decompress_.Decompress(in, inlen, out, outlen)) {
                if (inlen != 0) {
                    buffer_.IncrementContiguiousRead(inlen);
                }
                if (outlen != 0) {
                    next_->IncrementRecvData(outlen);
                }
            } else {
                THROW_EXCEPTION(RecvDataException());
            }
        }
    }
}
