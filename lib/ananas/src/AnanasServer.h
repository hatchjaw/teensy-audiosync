#ifndef ANANASSERVER_H
#define ANANASSERVER_H

#include <AnanasPacket.h>
#include <AnanasPacketBuffer.h>
#include <AudioProcessor.h>
#include <ProgramComponent.h>
#include <NetworkProcessor.h>
#include <Utils.h>

namespace ananas
{
    class AudioServer final : public AudioProcessor,
                              public ProgramComponent,
                              public NetworkProcessor
    {
    public:
        void begin() override;

        void run() override;

        void connect() override;

        void processAudio(int16_t *buffer, size_t numChannels, size_t numSamples) override;

        void prepare(uint sampleRate) override;

        void adjustBufferReadIndex(NanoTime now);

    private:
        void send();

        static constexpr int64_t kPacketReproductionOffsetNs{Constants::kNanoSecondsPerSecond / 20};
        static constexpr size_t kNumFrames{Constants::kAudioBlockFrames};

        bool connected{false};
        Packet txPacket = {};
        PacketBuffer packetBuffer;
        PacketBuffer playbackPacketBuffer;
        uint sampleRate{0};
    };
} // ananas

#endif //ANANASSERVER_H
