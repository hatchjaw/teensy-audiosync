#ifndef ANANASSERVER_H
#define ANANASSERVER_H

#include "AnanasPacket.h"
#include "AnanasPacketBuffer.h"
#include <AudioProcessor.h>
#include <ProgramComponent.h>
#include <NetworkProcessor.h>

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

        void prepare(uint sampleRate) override;

        void adjustBufferReadIndex(NanoTime now);

    private:
        void send();

    protected:
        void processImpl(int16_t *buffer, size_t numChannels, size_t numSamples) override;

    private:
        bool connected{false};
        AudioPacket txPacket = {};
        PacketBuffer packetBuffer;
        PacketBuffer playbackPacketBuffer;
        uint sampleRate{0};
    };
} // ananas

#endif //ANANASSERVER_H
