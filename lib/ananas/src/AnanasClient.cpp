#include "AnanasClient.h"
#include <QNEthernet.h>
#include "Utils.h"

namespace ananas
{
    void AudioClient::begin()
    {
    }

    void AudioClient::run()
    {
        if (const auto size{socket.parsePacket()}; size > 0) {
            if (size == kPacketSize) {
                nWrite++;

                socket.read(rxPacket.rawData(), size);
                packetBuffer.write(rxPacket);

                // if (nWrite % 1000 == 0) {
                //     Serial.println("Received packet:");
                //     Utils::hexDump(rxPacket->rawData(), size);
                // }

                if (nWrite > kReportThreshold) {
                    timespec now{};
                    qindesign::network::EthernetIEEE1588.readTimer(now);
                    const auto ns{now.tv_sec * Constants::kNanoSecondsPerSecond + now.tv_nsec};

                    const int64_t diff{ns - prevTime};
                    if (prevTime != 0) {
                        totalTime += diff;
                    }
                    prevTime = ns;
                }
            } else {
                Serial.printf("\n*** Received malformed packet of %d bytes.\n", size);
            }
        }
    }

    void AudioClient::connect()
    {
        socket.beginMulticast( Constants::kMulticastIP, Constants::kAudioPort);
    }

    void AudioClient::prepare(const uint sampleRate)
    {
        this->sampleRate = sampleRate;
        packetBuffer.clear();
    }

    size_t AudioClient::printTo(Print &p) const
    {
        return AudioProcessor::printTo(p)
               + (nWrite < kReportThreshold
                      ? p.printf("\nPackets received: %" PRIu32 "\n", nWrite)
                      : p.printf("\nPackets received: %" PRIu32 " Average reception interval: %e ns\n",
                                 nWrite,
                                 static_cast<double>(totalTime) / (nWrite - kReportThreshold)))
               + p.print(packetBuffer)
               + p.printf("Packet offset: %" PRId64 " ns, Sample offset: %" PRId32, packetOffset, sampleOffset);
    }

    void AudioClient::processImpl(int16_t *buffer, const size_t numChannels, const size_t numSamples)
    {
        nRead++;

        // // const auto audio{packetBuffer.read().audio()};

        auto [packetTime, audioData]{packetBuffer.read()};

        // if (nRead % 1000 <= 1) {
        //     Serial.println("Read audio:");
        //     Utils::hexDump(reinterpret_cast<uint8_t *>(audio), sizeof(int16_t) * numChannels * numSamples);
        // }

        // timespec ts{};
        // qindesign::network::EthernetIEEE1588.readTimer(ts);
        // const NanoTime now{ts.tv_sec * 1'000'000'000 + ts.tv_nsec}, diff{now - packetTime};
        // if (abs(diff) > 2'666'666) {
        //     if (std::signbit(diff)) {
        //         packetBuffer.decrementReadIndex();
        //     } else {
        //         packetBuffer.incrementReadIndex();
        //     }
        //     Serial.print("Current time: ");
        //     Utils::printTime(now);
        //     Serial.printf(", Read index: %" PRIu32 "\nPacket time:  ", packetBuffer.getReadIndex());
        //     Utils::printTime(packetTime);
        //     Serial.printf(", diff: %" PRId64 "\n", diff);
        // }

        memcpy(buffer, audioData, sizeof(int16_t) * numChannels * numSamples);
    }

    void AudioClient::adjustBufferReadIndex(const NanoTime now)
    {
        const auto idx{packetBuffer.getReadIndex()},
                initialReadIndex{(idx + PacketBuffer::kPacketBufferSize - 1) % PacketBuffer::kPacketBufferSize};
        auto packetTime{packetBuffer.peek().time};
        auto diff{now - packetTime};
        const auto kMaxDiff{static_cast<int64_t>(1e9 * kNumFrames / sampleRate)};
        // while (abs(diff) > kMaxDiff && initialReadIndex != packetBuffer.getReadIndex()) {
        while ((diff > kMaxDiff / 2 || diff < -kMaxDiff / 2) && initialReadIndex != packetBuffer.getReadIndex()) {
            // if (std::signbit(diff)) {
            //     packetBuffer.decrementReadIndex();
            // } else {
            packetBuffer.incrementReadIndex();
            // }
            packetTime = packetBuffer.peek().time;
            diff = now - packetTime;
            // Serial.print("Current time: ");
            // Utils::printTime(now);
            // Serial.printf(", Read index: %" PRIu32 "\nPacket time:  ", packetBuffer.getReadIndex());
            // Utils::printTime(packetTime);
            // Serial.printf(", diff: %" PRId64 "\n", diff, kMaxDiff);

            // Serial.printf("Read index: %" PRIu32 "\n", packetBuffer.getReadIndex());
        }
        packetOffset = diff;
        sampleOffset = diff / static_cast<int64_t>(1e9 / sampleRate);
    }
}
