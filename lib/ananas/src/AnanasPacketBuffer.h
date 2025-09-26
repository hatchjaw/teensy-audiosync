#ifndef ANANASPACKETBUFFER_H
#define ANANASPACKETBUFFER_H

#include <AnanasPacket.h>
#include <Arduino.h>

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

        static constexpr size_t kPacketBufferSize{50};

        virtual ~PacketBuffer() = default;

        void write(const Packet &packet);
        void writeV2(const PacketV2 &packet);

        Packet &read();
        PacketV2 &readV2();

        Packet &peek();
        PacketV2 &peekV2();

        size_t printTo(Print &p) const override;

        void incrementReadIndex();

        void decrementReadIndex();

        [[nodiscard]] size_t getReadIndex() const;

        [[nodiscard]] bool isEmpty() const;

        void clear();

    private:
        size_t writeIndex{0}, readIndex{0};
        Packet buffer[kPacketBufferSize] = {};
        PacketV2 bufferV2[kPacketBufferSize] = {};
    };
} // ananas

#endif //ANANASPACKETBUFFER_H
