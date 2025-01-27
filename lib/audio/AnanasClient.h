#ifndef ANANASCLIENT_H
#define ANANASCLIENT_H

#include <memory>
#include <AudioProcessor.h>
#include <QNEthernetUDP.h>
#include <t41-ptp.h>

namespace ananas
{
    /**
     * TODO: inherit from `Component`, which should provide virtual `begin` and `loop` methods.
     */
    class AudioClient final : public AudioProcessor
    {
    public:
        struct Packet
        {
            uint8_t *rawData() { return reinterpret_cast<uint8_t *>(this); }
            int16_t *audio() { return reinterpret_cast<int16_t *>(data); }
            NanoTime time;
            uint8_t data[128 << 2];
        };

        AudioClient();

        void begin();

        void loop();

        void connect();

        void processAudio(int16_t *dest, size_t numChannels, size_t numSamples) override;

        void printStats() const;

        void adjustBufferReadIndex(NanoTime now);

    private:
        class PacketBuffer final : public Printable
        {
        public:
            static constexpr size_t kPacketBufferSize{50};

            virtual ~PacketBuffer() = default;

            void write(const std::unique_ptr<Packet> &packet);

            Packet &read();

            Packet &peek();

            size_t printTo(Print &p) const override;

            void incrementReadIndex();

            void decrementReadIndex();

            size_t getReadIndex() const;

        private:
            size_t writeIndex{0}, readIndex{0};
            Packet packetBuffer[kPacketBufferSize] = {};
        };

        static constexpr size_t kNumChannels{2};
        static constexpr size_t kNumFrames{128};
        static constexpr size_t kSampleSize{sizeof(int16_t)};
        static constexpr size_t kHeaderSize{sizeof(NanoTime)};
        static constexpr size_t kPacketSize{kNumChannels * kNumFrames * kSampleSize + kHeaderSize};
        std::unique_ptr<Packet> rxPacket;
        PacketBuffer packetBuffer;

        qindesign::network::EthernetUDP socket;
        uint nWrite{0}, nRead{0};
    };
}

#endif //ANANASCLIENT_H
