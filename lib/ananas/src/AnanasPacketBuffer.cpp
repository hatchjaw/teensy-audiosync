#include "AnanasPacketBuffer.h"

namespace ananas
{
    void PacketBuffer::write(const AudioPacket &packet)
    {
        buffer[writeIndex] = packet;
        writeIndex = (writeIndex + 1) % Constants::PacketBufferCapacity;
    }

    AudioPacket &PacketBuffer::read()
    {
        auto &packet{peek()};
        incrementReadIndex();
        return packet;
    }

    AudioPacket &PacketBuffer::peek()
    {
        return buffer[readIndex];
    }

    size_t PacketBuffer::printTo(Print &p) const
    {
        return p.printf("  Read index: %" PRIu32 ", Write index: %" PRIu32 ", %" PRIu8 " %% full.\n",
                        readIndex,
                        writeIndex,
                        getFillPercent());
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

    void PacketBuffer::setReadIndex(const size_t newReadIndex)
    {
        readIndex = newReadIndex;
    }

    bool PacketBuffer::isEmpty() const
    {
        return readIndex == writeIndex;
    }

    void PacketBuffer::clear()
    {
        readIndex = 0;
        readIndex = 0;
        writeIndex = 0;
        writeIndex = 0;
        memset(buffer->data, 0, sizeof(buffer->data));
    }

    uint8_t PacketBuffer::getFillPercent() const
    {
        return 100 * (writeIndex > readIndex ? writeIndex - readIndex : writeIndex + Constants::PacketBufferCapacity - readIndex) /
                        Constants::PacketBufferCapacity;
    }
} // ananas
