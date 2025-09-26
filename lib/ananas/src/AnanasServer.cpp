#include "AnanasServer.h"

#include <AnanasPacket.h>
#include <QNEthernet.h>
#include <ptp/ptp-base.h>

namespace ananas
{
    void AudioServer::begin()
    {
    }

    void AudioServer::run()
    {
        if (!connected) return;

        send();
    }

    void AudioServer::connect()
    {
        connected = socket.beginMulticast({224, 4, 224, 4}, 49152);
        if (!connected) {
            Serial.println("AnanasServer: Failed to connect to multicast group");
        }
    }

    void AudioServer::prepare(const uint sampleRate)
    {
        this->sampleRate = sampleRate;
        packetBuffer.clear();
    }

    void AudioServer::send()
    {
        if (packetBuffer.isEmpty()) return;

        socket.beginPacket({224, 4, 224, 4}, 49152);
        socket.write(packetBuffer.read().rawData(), sizeof(Packet));
        socket.endPacket();
    }

    void AudioServer::processImpl(int16_t *buffer, size_t numChannels, size_t numSamples)
    {
        if (!connected) return;

        // Write incoming buffer to a packet in the packet buffer.
        memcpy(txPacket.data, buffer, numSamples * numChannels * sizeof(int16_t));

        // Get the current ethernet time.
        timespec ts{};
        qindesign::network::EthernetIEEE1588.readTimer(ts);
        const NanoTime now{ts.tv_sec * NS_PER_S + ts.tv_nsec};
        // Set packet time some way in the future.
        txPacket.time = now + kPacketReproductionOffsetNs;

        packetBuffer.write(txPacket);
        playbackPacketBuffer.write(txPacket);

        auto [packetTime, audioData]{playbackPacketBuffer.read()};
        memcpy(buffer, audioData, sizeof(int16_t) * numChannels * numSamples);
    }

    void AudioServer::adjustBufferReadIndex(const NanoTime now)
    {
        const auto idx{playbackPacketBuffer.getReadIndex()},
                initialReadIndex{(idx + PacketBuffer::kPacketBufferSize - 1) % PacketBuffer::kPacketBufferSize};
        auto packetTime{playbackPacketBuffer.peek().time};
        auto diff{now - packetTime};
        const auto kMaxDiff{static_cast<int64_t>(1e9 * kNumFrames / sampleRate)};
        // while (abs(diff) > kMaxDiff && initialReadIndex != packetBuffer.getReadIndex()) {
        while ((diff > kMaxDiff / 2 || diff < -kMaxDiff / 2) && initialReadIndex != playbackPacketBuffer.getReadIndex()) {
            // if (std::signbit(diff)) {
            //     packetBuffer.decrementReadIndex();
            // } else {
            playbackPacketBuffer.incrementReadIndex();
            // }
            packetTime = playbackPacketBuffer.peek().time;
            diff = now - packetTime;
            Serial.print("Current time: ");
            Utils::printTime(now);
            Serial.printf(", Read index: %" PRIu32 "\nPacket time:  ", playbackPacketBuffer.getReadIndex());
            Utils::printTime(packetTime);
            Serial.printf(", diff: %" PRId64 "\n", diff, kMaxDiff);
        }
        // sampleOffset = diff / static_cast<int64_t>(1e9 / sampleRate);
        // Serial.printf("Packet offset: %" PRId64 " ns, Sample offset: %" PRId32 "\n", diff, sampleOffset);
    }
} // ananas
