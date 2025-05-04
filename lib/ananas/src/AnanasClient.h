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
                              public NetworkProcessor,
                              public Printable
    {
    public:
        void begin() override;

        void run() override;

        void connect() override;

        void processAudio(int16_t *buffer, size_t numChannels, size_t numSamples) override;

        void adjustBufferReadIndex(NanoTime now);

        void prepare(uint sampleRate) override;

        size_t printTo(Print &p) const override;

    private:
        static constexpr size_t kNumChannels{2};
        static constexpr size_t kNumFrames{Constants::kAudioBlockFrames};
        static constexpr size_t kSampleSize{sizeof(int16_t)};
        static constexpr size_t kHeaderSize{sizeof(NanoTime)};
        static constexpr size_t kPacketSize{kNumChannels * kNumFrames * kSampleSize + kHeaderSize};

        static constexpr uint kReportThreshold{5000};

        Packet rxPacket{};
        PacketBuffer packetBuffer;
        int sampleOffset{0};
        int64_t packetOffset{0};

        uint nWrite{0}, nRead{0};
        NanoTime prevTime{0}, totalTime{0};
        uint sampleRate{0};
    };
}

#endif //ANANASCLIENT_H
