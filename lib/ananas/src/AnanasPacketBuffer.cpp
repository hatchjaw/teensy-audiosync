#include "AnanasPacketBuffer.h"

namespace ananas
{
    void PacketBuffer::write(const Packet &packet)
    {
        // memcpy(&packetBuffer[writeIndex], packet->rawData(), sizeof(NanoTime) + (128 << 2));
        buffer[writeIndex] = packet;
        writeIndex = (writeIndex + 1) % kPacketBufferSize;
        // Might be better to avoid a division...
        // ++writeIndex;
        // if (writeIndex >= kPacketBufferSize) {
        // writeIndex = 0;
        // }
    }

    void PacketBuffer::writeV2(const PacketV2 &packet)
    {
        bufferV2[writeIndex] = packet;
        writeIndex = (writeIndex + 1) % kPacketBufferSize;
    }

    Packet &PacketBuffer::read()
    {
        auto &packet{peek()};
        incrementReadIndex();
        return packet;
    }

    PacketV2 &PacketBuffer::readV2()
    {
        auto &packet{peekV2()};
        incrementReadIndex();
        return packet;
    }

    Packet &PacketBuffer::peek()
    {
        return buffer[readIndex];
    }

    PacketV2 &PacketBuffer::peekV2()
    {
        return bufferV2[readIndex];
    }

    size_t PacketBuffer::printTo(Print &p) const
    {
        return p.printf("Read index: %" PRIu32 ", Write index: %" PRIu32 ", Num packets available: %" PRIu32 "\n",
                        readIndex,
                        writeIndex,
                        writeIndex > readIndex ? writeIndex - readIndex : writeIndex + kPacketBufferSize - readIndex);
    }

    void PacketBuffer::incrementReadIndex()
    {
        readIndex = (readIndex + 1) % kPacketBufferSize;
        // Might be better to avoid a division...
        // ++readIndex;
        // if (readIndex >= kPacketBufferSize) {
        // readIndex = 0;
        // }
    }

    void PacketBuffer::decrementReadIndex()
    {
        readIndex = (readIndex + kPacketBufferSize - 1) % kPacketBufferSize;
        // Might be better to avoid a division...
        // if (readIndex == 0) {
        // readIndex = kPacketBufferSize;
        // }
        // --readIndex;
    }

    size_t PacketBuffer::getReadIndex() const
    {
        return readIndex;
    }

    bool PacketBuffer::isEmpty() const
    {
        return readIndex == writeIndex;
    }

    void PacketBuffer::clear()
    {
        readIndex = 0;
        writeIndex = 0;
        memset(buffer, 0, sizeof(buffer));
    }
} // ananas
