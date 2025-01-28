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

    private:
        void send();

        static constexpr int64_t kPacketReproductionOffsetNs{Constants::kNanoSecondsPerSecond / 10};

        bool connected{false};
        Packet txPacket = {};
        PacketBuffer packetBuffer;
    };
} // ananas

#endif //ANANASSERVER_H
