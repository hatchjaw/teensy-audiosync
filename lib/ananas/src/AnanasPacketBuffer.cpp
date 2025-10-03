#include "AnanasPacketBuffer.h"

namespace ananas
{
    void PacketBuffer::write(const Packet &packet)
    {
        buffer[writeIndex] = packet;
        writeIndex = (writeIndex + 1) % Constants::PacketBufferCapacity;
        // Might be better to avoid a division...
        // ++writeIndex;
        // if (writeIndex >= kPacketBufferSize) {
        // writeIndex = 0;
        // }
    }

    void PacketBuffer::writeV2(const PacketV2 &packet)
    {
        bufferV2[writeIndex] = packet;
        writeIndex = (writeIndex + 1) % Constants::PacketBufferCapacity;
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
        incrementReadIndexV2();
        return packet;
    }

    Packet &PacketBuffer::peek()
    {
        return buffer[readIndex];
    }

    PacketV2 &PacketBuffer::peekV2()
    {
        return bufferV2[readIndexV2];
    }

    size_t PacketBuffer::printTo(Print &p) const
    {
        return p.printf("Read index: %" PRIu32 ", Write index: %" PRIu32 ", Num packets available: %" PRIu32 "\n",
                        readIndexV2,
                        writeIndex,
                        writeIndex > readIndexV2 ? writeIndex - readIndexV2 : writeIndex + Constants::PacketBufferCapacity - readIndexV2);
    }

    void PacketBuffer::incrementReadIndex()
    {
        readIndex = (readIndex + 1) % Constants::PacketBufferCapacity;
        // Might be better to avoid a division...
        // ++readIndex;
        // if (readIndex >= kPacketBufferSize) {
        // readIndex = 0;
        // }
    }

    void PacketBuffer::incrementReadIndexV2()
    {
        readIndexV2 = (readIndexV2 + 1) % Constants::PacketBufferCapacity;
    }

    void PacketBuffer::decrementReadIndex()
    {
        readIndex = (readIndex + Constants::PacketBufferCapacity - 1) % Constants::PacketBufferCapacity;
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

    size_t PacketBuffer::getReadIndexV2() const
    {
        return readIndexV2;
    }

    bool PacketBuffer::isEmpty() const
    {
        return readIndex == writeIndex;
    }

    void PacketBuffer::clear()
    {
        readIndex = 0;
        readIndexV2 = 0;
        writeIndex = 0;
        writeIndexV2 = 0;
        memset(buffer, 0, sizeof(buffer));
        memset(bufferV2, 0, sizeof(bufferV2));
    }
} // ananas
