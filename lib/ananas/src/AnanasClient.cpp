#include "AnanasClient.h"
#include "AnanasPacketBuffer.h"
#include "AnanasUtils.h"
#include <QNEthernet.h>

namespace ananas
{
    AudioClient::AudioClient(): AudioProcessor(0, Constants::MaxChannels),
                                announcer(Constants::ClientAnnouncePort, Constants::ClientAnnounceIntervalMs)
    {
    }

    void AudioClient::begin()
    {
        announcer.begin();
    }

    void AudioClient::run()
    {
        if (const auto size{socket.parsePacket()}; size > 0) {
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
        }

        announcer.run();
    }

    void AudioClient::connect()
    {
        socket.beginMulticast(Constants::MulticastIP, Constants::AudioPort);
        announcer.connect();
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
                      ? p.printf("\nPackets received: %" PRIu32 "\n", nWrite)
                      : p.printf("\nPackets received: %" PRIu32 " Average reception interval: %e ns\n",
                                 nWrite,
                                 static_cast<double>(totalTime) / (nWrite - Constants::ClientReportThresholdPackets)))
               + p.print(packetBuffer)
               + p.printf("Packet offset: %" PRId64 " ns (%" PRId32
                          " frames), Times adjusted: %" PRIu16,
                          announcer.txPacket.presentationOffsetNs,
                          announcer.txPacket.presentationOffsetFrame,
                          numPacketBufferReadIndexAdjustments);
    }

    void AudioClient::setExactSamplingRate(const double samplingRate)
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

    void AudioClient::processImpl(int16_t *buffer, const size_t numChannels, const size_t numFrames)
    {
        // auto [packetTime, audioData]{packetBuffer.read()};
        auto [header, audioData]{packetBuffer.read()};

        memcpy(buffer, audioData, sizeof(int16_t) * numChannels * numFrames);
    }

    void AudioClient::processImplV2(const size_t numFrames)
    {
        if (mute) {
            for (size_t channel{0}; channel < numOutputs; ++channel) {
                memset(outputBuffer[channel], 0, sizeof(int16_t) * numFrames);
            }
            return;
        }

        size_t processFrame{0};
        while (processFrame < numFrames) {
            auto packet{packetBuffer.read()};

            const size_t numChannels{min(packet.header.numChannels, numOutputs)};
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
}
