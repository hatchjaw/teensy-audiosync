#ifndef ANANASCLIENT_H
#define ANANASCLIENT_H

#include "AnanasPacket.h"
#include "AnanasPacketBuffer.h"
#include <AudioProcessor.h>
#include <ProgramComponent.h>
#include <NetworkProcessor.h>
#include <Announcer.h>

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

        void adjustBufferReadIndex(NanoTime ptpNow);

        void prepare(uint sampleRate) override;

        size_t printTo(Print &p) const override;

        void setExactSamplingRate(double samplingRate);

        uint32_t getSerialNumber() const;

        void setIsPtpLocked(bool ptpLock);

        void setAudioPtpOffsetNs(long offset);

    protected:
        void processImpl(int16_t *buffer, size_t numChannels, size_t numFrames) override;

        void processImplV2(size_t numFrames) override;

    private:
        AudioPacket rxPacket{};
        PacketBuffer packetBuffer;

        uint nWrite{0}, nRead{0};
        NanoTime prevTime{0}, totalTime{0};
        uint sampleRate{0};

        bool mute{false};
        uint16_t numPacketBufferReadIndexAdjustments{0};

        Announcer<ClientAnnouncePacket> announcer;
    };
}

#endif //ANANASCLIENT_H
