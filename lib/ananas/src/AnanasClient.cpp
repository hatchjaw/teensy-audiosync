#include "AnanasClient.h"
#include "AnanasPacketBuffer.h"
#include "AnanasUtils.h"
#include <QNEthernet.h>

namespace ananas
{
    AudioClient::AudioClient(): AudioProcessor(0, Constants::MaxChannels)
    {
        getSerialNumber();
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

            announcer.txPacket.bufferFillPercent = packetBuffer.getFillPercent();
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
                          announcer.txPacket.offsetTime,
                          announcer.txPacket.offsetFrame,
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

    void AudioClient::adjustBufferReadIndex(const NanoTime now)
    {
        auto didAdjust{false};

        const auto idx{packetBuffer.getReadIndex()};
        const auto initialReadIndex{(idx + Constants::PacketBufferCapacity - 1) % Constants::PacketBufferCapacity};
        auto packetTime{packetBuffer.peek().header.time};
        auto diff{now - packetTime};
        const auto kMaxDiff{(Constants::NanosecondsPerSecond * Constants::FramesPerPacketExpected / sampleRate) * 3 / 2};

        while ((diff > kMaxDiff || diff < -kMaxDiff) && packetBuffer.getReadIndex() != initialReadIndex) {
            packetBuffer.incrementReadIndex();

            packetTime = packetBuffer.peek().header.time;
            diff = now - packetTime;

            Serial.printf("Current diff %" PRId64 " (readIndex %d, initialReadIndex %d)\n", diff, packetBuffer.getReadIndex(), initialReadIndex);
            Serial.printf("Diff: %" PRId64 ", (max allowed ±%" PRId64 ")\n", diff, kMaxDiff);
            didAdjust = true;
        }

        if (didAdjust) {
            ++numPacketBufferReadIndexAdjustments;
        }
        mute = didAdjust;

        announcer.txPacket.offsetTime = diff;
        announcer.txPacket.offsetFrame = diff / static_cast<int64_t>(1e9 / sampleRate);
    }

    //==========================================================================
    AudioClient::ClientAnnouncer::ClientAnnouncer()
    {
        uint32_t num{HW_OCOTP_MAC0 & 0xFFFFFF};
        if (num < 10000000) num *= 10;
        txPacket.serial = num;
    }

    void AudioClient::ClientAnnouncer::begin()
    {
    }

    void AudioClient::ClientAnnouncer::run()
    {
        if (elapsed > Constants::ClientAnnounceIntervalMs) {
            socket.beginPacket(Constants::MulticastIP, Constants::AnnouncementPort);
            socket.write(txPacket.rawData(), sizeof(AnnouncementPacket));
            socket.endPacket();
            elapsed = 0;
        }
    }

    void AudioClient::ClientAnnouncer::connect()
    {
        socket.beginMulticast(Constants::MulticastIP, Constants::AnnouncementPort);
    }
}
