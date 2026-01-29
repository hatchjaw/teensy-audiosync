#include "AnanasClient.h"
#include "AnanasPacketBuffer.h"
#include "AnanasUtils.h"
#include <QNEthernet.h>

namespace ananas
{
    AudioClient::AudioClient() : announcer(Constants::ClientAnnounceMulticastIP,
                                           Constants::ClientAnnouncePort,
                                           Constants::ClientAnnounceIntervalMs)
    {
    }

    void AudioClient::begin()
    {
        announcer.begin();
    }

    void AudioClient::run()
    {
        int size{socket.parsePacket()};
        while (size > 0) {
            nWrite++;

            socket.read(rxPacket.rawData(), size);
            packetBuffer.write(rxPacket);

            if (nWrite > Constants::ClientReportThresholdPackets) {
                timespec now{};
                qindesign::network::EthernetIEEE1588.readTimer(now);
                const auto ns{now.tv_sec * Constants::NanosecondsPerSecond + now.tv_nsec};

                const int64_t diff{ns - prevTime};
                if (prevTime != 0) {
                    totalTime += diff;
                }
                prevTime = ns;
            }

            announcer.txPacket.bufferFillPercent = announcer.txPacket.ptpLock ? packetBuffer.getFillPercent() : 50;
            announcer.txPacket.percentCPU = getCurrentPercentCPU();
            size = socket.parsePacket();
        }

        announcer.run();
        rebootListener.run();
    }

    void AudioClient::connect()
    {
        socket.beginMulticast(Constants::AudioMulticastIP, Constants::AudioPort);
        announcer.connect();
        rebootListener.connect();
    }

    void AudioClient::prepare(const uint sampleRate)
    {
        this->sampleRate = sampleRate;
        packetBuffer.clear();
    }

    size_t AudioClient::printTo(Print &p) const
    {
        return p.print("Ananas Client:     ") + AudioProcessor::printTo(p)
               + (nWrite < Constants::ClientReportThresholdPackets
                      ? p.printf("\n  Packets received: %" PRIu32 "\n", nWrite)
                      : p.printf("\n  Packets received: %" PRIu32 " Average reception interval: %e ns\n",
                                 nWrite,
                                 static_cast<double>(totalTime) / (nWrite - Constants::ClientReportThresholdPackets)))
               + p.print(packetBuffer)
               + p.printf("  Packet offset: %" PRId64 " ns (%" PRId32
                          " frames), Times adjusted: %" PRIu16,
                          announcer.txPacket.presentationOffsetNs,
                          announcer.txPacket.presentationOffsetFrame,
                          numPacketBufferReadIndexAdjustments);
    }

    void AudioClient::setReportedSamplingRate(const double samplingRate)
    {
        announcer.txPacket.samplingRate = samplingRate;
    }

    uint32_t AudioClient::getSerialNumber() const
    {
        return announcer.txPacket.serial;
    }

    void AudioClient::setIsPtpLocked(const bool ptpLock)
    {
        announcer.txPacket.ptpLock = ptpLock;
    }

    void AudioClient::setAudioPtpOffsetNs(const long offset)
    {
        announcer.txPacket.audioPtpOffsetNs = offset;
        announcer.txPacket.presentationOffsetFrame = (announcer.txPacket.presentationOffsetNs + offset) / static_cast<int64_t>(1e9 / sampleRate);
    }

    void AudioClient::setPercentCPU(const float percentage)
    {
        announcer.txPacket.percentCPU = percentage;
    }

    void AudioClient::setModuleID(const uint16_t moduleID)
    {
        announcer.txPacket.moduleID = moduleID;
    }

    void AudioClient::processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames)
    {
        if (mute) {
            for (size_t channel{0}; channel < getNumOutputs(); ++channel) {
                memset(outputBuffer[channel], 0, sizeof(int16_t) * numFrames);
            }
            return;
        }

        size_t processFrame{0};
        while (processFrame < numFrames) {
            auto packet{packetBuffer.read()};

            const size_t numChannels{min(packet.header.numChannels, getNumOutputs())};
            const auto audioData{packet.audio()};

            for (size_t packetFrame{0}; packetFrame < packet.header.numFrames; ++packetFrame, ++processFrame) {
                for (size_t channel{0}; channel < numChannels; ++channel) {
                    outputBuffer[channel][processFrame] = audioData[packetFrame * numChannels + channel];
                }
            }
        }
    }

    void AudioClient::adjustBufferReadIndex(const NanoTime ptpNow)
    {
        auto didAdjust{false};

        auto diff{ptpNow - packetBuffer.peek().header.time};
        const auto kMaxDiff{(Constants::NanosecondsPerSecond * Constants::FramesPerPacketExpected / sampleRate)};

        if (diff < 0 || diff > kMaxDiff) {
            Serial.printf("\nPresentation time diff %" PRId64 " ns outside of range 0-%" PRId64 " ns\n"
                          "Seeking closest packet... ", diff, kMaxDiff);
            auto minDiff{INT64_MAX};
            size_t readIndex{0};
            packetBuffer.setReadIndex(readIndex);

            // Seek the packet with the smallest positive presentation time
            // offset with respect to PTP time.
            for (size_t frame{0}; frame < Constants::PacketBufferCapacity; ++frame, packetBuffer.incrementReadIndex()) {
                diff = ptpNow - packetBuffer.peek().header.time;
                if (diff >= 0 && diff <= minDiff) {
                    minDiff = diff;
                    readIndex = packetBuffer.getReadIndex();
                }
            }

            if (minDiff < kMaxDiff) {
                Serial.printf("Found packet with presentation time diff %" PRId64 " ns (read index %d)\n", minDiff, readIndex);
                packetBuffer.setReadIndex(readIndex);
            } else {
                Serial.printf("Unable to find a packet with a valid presentation time (min %" PRId64 " ns)\n", minDiff);
                packetBuffer.setReadIndex(0);
            }

            didAdjust = true;
        }

        if (didAdjust) {
            ++numPacketBufferReadIndexAdjustments;
        }
        mute = didAdjust;

        announcer.txPacket.presentationOffsetNs = diff;
    }

    //==========================================================================

    void AudioClient::RebootListener::begin()
    {
    }

    void AudioClient::RebootListener::run()
    {
        if (const auto size{socket.parsePacket()}; size == 0) {
            Serial.println("Rebooting.");
            SRC_GPR5 = 0x0BAD00F1;
            SCB_AIRCR = 0x05FA0004;
            while (true) {
            }
        }
    }

    void AudioClient::RebootListener::connect()
    {
        socket.beginMulticast(Constants::RebootMulticastIP, Constants::RebootPort);
    }
}
