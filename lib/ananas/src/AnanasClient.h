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
    class ListenerSocket : public NetworkProcessor
    {
    public:
        explicit ListenerSocket(const SocketParams &p);

        void connect() override;
    private:
        IPAddress ip;
        uint16_t port;
    };

    class AudioClient final : public ListenerSocket,
                              public AudioProcessor,
                              public ProgramComponent
    {
    protected:
        void beginImpl() override;

    public:
        explicit AudioClient(const SocketParams &p);

        void run() override;

        void connect() override;

        void adjustBufferReadIndex(NanoTime ptpNow);

        void prepare(uint sampleRate) override;

        size_t printTo(Print &p) const override;

        void setReportedSamplingRate(double samplingRate);

        void setIsPtpLocked(bool ptpLock);

        void setAudioPtpOffsetNs(long offset);

        [[nodiscard]] size_t getNumInputs() const override { return 0; }

        [[nodiscard]] size_t getNumOutputs() const override { return Constants::MaxChannels; }

        void setPercentCPU(float percentage);

        void setModuleID(uint16_t moduleID);

    protected:
        void processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames) override;

    private:
        class RebootListener final : public ListenerSocket,
                                     public ProgramComponent
        {
        protected:
            void beginImpl() override;

        public:
            explicit RebootListener(const SocketParams &p);

            void run() override;

            size_t printTo(Print &p) const override;
        };

        AudioPacket rxPacket{};
        PacketBuffer packetBuffer;

        uint nWrite{0}, nRead{0};
        NanoTime prevTime{0}, totalTime{0};
        uint sampleRate{0};

        bool mute{false};
        uint16_t numPacketBufferReadIndexAdjustments{0};

        Announcer<ClientAnnouncePacket> announcer;
        RebootListener rebootListener;
    };
}

#endif //ANANASCLIENT_H
