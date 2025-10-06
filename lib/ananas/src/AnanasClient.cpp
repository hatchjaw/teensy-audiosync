#include "AnanasClient.h"

#include <AnanasPacketBuffer.h>
#include <QNEthernet.h>
#include <AnanasUtils.h>

namespace ananas
{
    AudioClient::AudioClient(): AudioProcessor(0, Constants::MaxChannels)
    {
    }

    void AudioClient::begin()
    {
    }

    void AudioClient::run()
    {
        if (const auto size{socket.parsePacket()}; size > 0) {
            // if (size == kPacketSize) {
            //     nWrite++;
            //
            //     socket.read(rxPacket.rawData(), size);
            //     packetBuffer.write(rxPacket);
            //
            //     // if (nWrite % 1000 == 0) {
            //     //     Serial.println("Received packet:");
            //     //     Utils::hexDump(rxPacket->rawData(), size);
            //     // }
            //
            //     if (nWrite > kReportThreshold) {
            //         timespec now{};
            //         qindesign::network::EthernetIEEE1588.readTimer(now);
            //         const auto ns{now.tv_sec * Constants::kNanoSecondsPerSecond + now.tv_nsec};
            //
            //         const int64_t diff{ns - prevTime};
            //         if (prevTime != 0) {
            //             totalTime += diff;
            //         }
            //         prevTime = ns;
            //     }
            // } else {
            //     Serial.printf("\n*** Received malformed packet of %d bytes.\n", size);
            // }

            nWrite++;

            socket.read(rxPacketV2.rawData(), size);
            packetBuffer.writeV2(rxPacketV2);

            if (nWrite > Constants::ClientReportThreshold) {
                timespec now{};
                qindesign::network::EthernetIEEE1588.readTimer(now);
                const auto ns{now.tv_sec * Constants::NanosecondsPerSecond + now.tv_nsec};

                const int64_t diff{ns - prevTime};
                if (prevTime != 0) {
                    totalTime += diff;
                }
                prevTime = ns;
            }
        }
    }

    void AudioClient::connect()
    {
        socket.beginMulticast(Constants::MulticastIP, Constants::AudioPort);
    }

    void AudioClient::prepare(const uint sampleRate)
    {
        this->sampleRate = sampleRate;
        packetBuffer.clear();
    }

    size_t AudioClient::printTo(Print &p) const
    {
        return AudioProcessor::printTo(p)
               + (nWrite < Constants::ClientReportThreshold
                      ? p.printf("\nPackets received: %" PRIu32 "\n", nWrite)
                      : p.printf("\nPackets received: %" PRIu32 " Average reception interval: %e ns\n",
                                 nWrite,
                                 static_cast<double>(totalTime) / (nWrite - Constants::ClientReportThreshold)))
               + p.print(packetBuffer)
               + p.printf("Packet offset: %" PRId64 " ns, Sample offset: %" PRId32, packetOffset, sampleOffset);
    }

    void AudioClient::processImpl(int16_t *buffer, const size_t numChannels, const size_t numFrames)
    {
        // auto [packetTime, audioData]{packetBuffer.read()};
        auto [header, audioData]{packetBuffer.readV2()};

        memcpy(buffer, audioData, sizeof(int16_t) * numChannels * numFrames);
    }

    void AudioClient::processImplV2(const size_t numFrames)
    {
        size_t processFrame{0};
        while (processFrame < numFrames) {
            auto packet{packetBuffer.readV2()};

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
        // auto didAdjust{false};
        // NanoTime minDiff{1'000'000'000}, maxDiff{-1'000'000'000};

        const auto idx{packetBuffer.getReadIndexV2()};
        const auto initialReadIndex{(idx + Constants::PacketBufferCapacity - 1) % Constants::PacketBufferCapacity};
        auto packetTime{packetBuffer.peekV2().header.time};
        auto diff{now - packetTime};
        // const auto kMaxDiff{(Constants::NanosecondsPerSecond * Constants::AudioBlockFrames / sampleRate)};
        const auto kMaxDiff{(Constants::NanosecondsPerSecond * Constants::FramesPerPacketExpected / sampleRate) * 3 / 2};
        // while (abs(diff) > kMaxDiff && initialReadIndex != packetBuffer.getReadIndex()) {

        while ((diff > kMaxDiff || diff < -kMaxDiff) && initialReadIndex != packetBuffer.getReadIndexV2()) {
            // didAdjust = true;

            packetBuffer.incrementReadIndexV2();

            packetTime = packetBuffer.peekV2().header.time;
            diff = now - packetTime;

            // if (diff < minDiff) {
            //     minDiff = diff;
            // } else if (diff > maxDiff) {
            //     maxDiff = diff;
            // }

            // Serial.print("Current time: ");
            // printTime(now);
            // Serial.printf("Packet time:  ");
            // printTime(packetTime);
            Serial.printf("Diff: %" PRId64 ", (max allowed ±%" PRId64 ")\n", diff, kMaxDiff);
            // Serial.printf("Read index: %" PRIu32 "\n", packetBuffer.getReadIndexV2());
        }

        // if (didAdjust && initialReadIndex == packetBuffer.getReadIndexV2()) {
        //     Serial.printf("\n*** Unable to find a suitable packet! Timestamp range %" PRId64 " to %" PRId64 " ns\n", minDiff, maxDiff);
        // } else if (didAdjust) {
        //     Serial.printf("\n*** Changed packet buffer read index from %d to %d\n", initialReadIndex, packetBuffer.getReadIndexV2());
        // }

        packetOffset = diff;
        sampleOffset = diff / static_cast<int64_t>(1e9 / sampleRate);
    }
}
