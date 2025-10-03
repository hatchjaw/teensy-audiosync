#ifndef ANANASCLIENT_H
#define ANANASCLIENT_H

#include <AudioProcessor.h>
#include <ProgramComponent.h>
#include <NetworkProcessor.h>
#include <AnanasPacket.h>
#include <AnanasPacketBuffer.h>

namespace ananas
{
    class AudioClient final : public AudioProcessor,
                              public ProgramComponent,
                              public NetworkProcessor
    {
    public:
        AudioClient();

        void begin() override;

        void run() override;

        void connect() override;

        void adjustBufferReadIndex(NanoTime now);

        void prepare(uint sampleRate) override;

        size_t printTo(Print &p) const override;

    protected:
        void processImpl(int16_t *buffer, size_t numChannels, size_t numFrames) override;

        void processImplV2(size_t numFrames) override;

    private:
        static constexpr size_t kNumChannels{2};
        static constexpr size_t kPacketSize{kNumChannels * Constants::AudioBlockFrames * Constants::SampleSizeBytes + sizeof(Packet::time)};

        Packet rxPacket{};
        PacketV2 rxPacketV2{};
        PacketBuffer packetBuffer;
        int sampleOffset{0};
        int64_t packetOffset{0};

        uint nWrite{0}, nRead{0};
        NanoTime prevTime{0}, totalTime{0};
        uint sampleRate{0};
    };
}

#endif //ANANASCLIENT_H
