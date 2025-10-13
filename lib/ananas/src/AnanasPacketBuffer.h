#ifndef ANANASPACKETBUFFER_H
#define ANANASPACKETBUFFER_H

#include "AnanasPacket.h"

namespace ananas
{
    class PacketBuffer final : public Printable
    {
    public:
        // Buffer fun https://www.snellman.net/blog/archive/2016-12-13-ring-buffers/
        // Or something like (decrementing)
        // index = (index > 0 ? index - 1 : bufferSize - 1) & (bufferSize - 1);
        // (incrementing)
        // index = (index + 1) & (kPacketBufferSize - 1);
        // That mask requires a power of two buffer size.
        // enum class Operation { Read, Write };

        virtual ~PacketBuffer() = default;

        void write(const AudioPacket &packet);

        AudioPacket &read();

        AudioPacket &peek();

        size_t printTo(Print &p) const override;

        void incrementReadIndex();

        void decrementReadIndex();

        [[nodiscard]] size_t getReadIndex() const;

        [[nodiscard]] bool isEmpty() const;

        void clear();

        uint8_t getFillPercent() const;

    private:
        size_t writeIndex{0}, readIndex{0};
        AudioPacket buffer[Constants::PacketBufferCapacity] = {};
    };
} // ananas

#endif //ANANASPACKETBUFFER_H
