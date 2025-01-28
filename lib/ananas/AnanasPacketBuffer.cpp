#include "AnanasPacketBuffer.h"

namespace ananas {
    void PacketBuffer::write(const Packet &packet)
    {
        // memcpy(&packetBuffer[writeIndex], packet->rawData(), sizeof(NanoTime) + (128 << 2));
        packetBuffer[writeIndex] = packet;
        writeIndex = (writeIndex + 1) % kPacketBufferSize;
        // Might be better to avoid a division...
        // ++writeIndex;
        // if (writeIndex >= kPacketBufferSize) {
        // writeIndex = 0;
        // }
    }

    Packet &PacketBuffer::read()
    {
        auto &packet{peek()};
        incrementReadIndex();
        return packet;
    }

    Packet &PacketBuffer::peek()
    {
        return packetBuffer[readIndex];
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

    bool PacketBuffer::isFull() const
    {
        return readIndex == writeIndex;
    }
} // ananas
