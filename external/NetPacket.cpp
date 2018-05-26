#include "NetPacket.h"

void INetPacket::InitNetPacketPool()
{
    TNetPacket<32>::InitPool();
    TNetPacket<64>::InitPool();
    TNetPacket<128>::InitPool();
    TNetPacket<256>::InitPool();
    TNetPacket<512>::InitPool();
    TNetPacket<1024>::InitPool();
    TNetPacket<4096>::InitPool();
    TNetPacket<8192>::InitPool();
    TNetPacket<32768>::InitPool();
    TNetPacket<65536>::InitPool();
}

void INetPacket::ClearNetPacketPool()
{
    TNetPacket<32>::ClearPool();
    TNetPacket<64>::ClearPool();
    TNetPacket<128>::ClearPool();
    TNetPacket<256>::ClearPool();
    TNetPacket<512>::ClearPool();
    TNetPacket<1024>::ClearPool();
    TNetPacket<4096>::ClearPool();
    TNetPacket<8192>::ClearPool();
    TNetPacket<32768>::ClearPool();
    TNetPacket<65536>::ClearPool();
}

INetPacket *INetPacket::New(uint32 opcode, size_t size)
{
    if (size <= 32) return new TNetPacket<32>(opcode);
    if (size <= 64) return new TNetPacket<64>(opcode);
    if (size <= 128) return new TNetPacket<128>(opcode);
    if (size <= 256) return new TNetPacket<256>(opcode);
    if (size <= 512) return new TNetPacket<512>(opcode);
    if (size <= 1024) return new TNetPacket<1024>(opcode);
    if (size <= 4096) return new TNetPacket<4096>(opcode);
    if (size <= 8192) return new TNetPacket<8192>(opcode);
    if (size <= 32768) return new TNetPacket<32768>(opcode);
    return new TNetPacket<65536>(opcode);
}

void INetPacket::WriteHeader(const Header &header)
{
    if (header.len > MAX_NET_PACKET_SIZE) {
        THROW_EXCEPTION(NetStreamException());
    }

    Write<uint16>((uint16)header.len);
    Write<uint16>(header.cmd);
}

void INetPacket::ReadHeader(Header &header)
{
    header.len = Read<uint16>();
    header.cmd = Read<uint16>();

    if (header.len < Header::SIZE) {
        THROW_EXCEPTION(NetStreamException());
    }
}

INetPacket *INetPacket::Clone() const
{
    INetPacket *pck = New(GetOpcode(), GetTotalSize());
    pck->Append(GetBuffer(), GetTotalSize());
    pck->AdjustReadPos(GetReadPos());
    return pck;
}

INetPacket *INetPacket::ReadPacket()
{
    Header header;
    ReadHeader(header);

    INetPacket *pck = New(header.cmd, header.len - Header::SIZE);
    pck->Erlarge(header.len - Header::SIZE);
    Take((void*)pck->GetBuffer(), pck->GetTotalSize());

    return pck;
}

INetPacket &INetPacket::ReadPacket(INetPacket &pck)
{
    Header header;
    ReadHeader(header);

    pck.Reset(header.cmd);
    pck.Erlarge(header.len - Header::SIZE);
    Take((void*)pck.GetBuffer(), pck.GetTotalSize());

    return pck;
}

INetPacket &INetPacket::UnpackPacket()
{
    Header header;
    ReadHeader(header);

    SetOpcode(header.cmd);
    if (header.len != ((GetReadableSize() + Header::SIZE) & 0xffff)) {
        THROW_EXCEPTION(NetStreamException());
    }

    return *this;
}

void INetPacket::WritePacket(const INetPacket &other)
{
    WriteHeader(Header(other.GetOpcode(), other.GetReadableSize() + Header::SIZE));
    Append(other.GetReadableBuffer(), other.GetReadableSize());
}
