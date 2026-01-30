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
    protected:
        void beginImpl() override;

    public:
        void run() override;

        void connect() override;

        void prepare(uint sampleRate) override;

        void adjustBufferReadIndex(NanoTime now);

        size_t printTo(Print &p) const override;

    private:
        void send();

    protected:
        void processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames) override;

    private:
        bool connected{false};
        AudioPacket txPacket = {};
        PacketBuffer packetBuffer;
        PacketBuffer playbackPacketBuffer;
        uint sampleRate{0};
    };
} // ananas

#endif //ANANASSERVER_H
