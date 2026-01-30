#include "AnanasServer.h"
#include "AnanasPacket.h"
#include <QNEthernet.h>
#include <ptp/ptp-base.h>

namespace ananas
{
    void AudioServer::beginImpl()
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
        socket.write(packetBuffer.read().rawData(), sizeof(AudioPacket));
        socket.endPacket();
    }

    void AudioServer::processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames)
    {
        if (!connected) return;

        // Write incoming buffer to a packet in the packet buffer.
        memcpy(txPacket.data, inputBuffer, numFrames * getNumInputs() * sizeof(int16_t));

        // Get the current ethernet time.
        timespec ts{};
        qindesign::network::EthernetIEEE1588.readTimer(ts);
        const NanoTime now{ts.tv_sec * NS_PER_S + ts.tv_nsec};
        // Set packet time some way in the future.
        txPacket.header.time = now + Constants::PacketReproductionOffsetNs;

        packetBuffer.write(txPacket);
        playbackPacketBuffer.write(txPacket);

        auto [header, audioData]{playbackPacketBuffer.read()};
        memcpy(outputBuffer, audioData, sizeof(int16_t) * getNumOutputs() * numFrames);
    }

    void AudioServer::adjustBufferReadIndex(const NanoTime now)
    {
        const auto idx{playbackPacketBuffer.getReadIndex()},
                initialReadIndex{(idx + Constants::PacketBufferCapacity - 1) % Constants::PacketBufferCapacity};
        auto packetTime{playbackPacketBuffer.peek().header.time};
        auto diff{now - packetTime};
        const auto kMaxDiff{static_cast<int64_t>(Constants::NanosecondsPerSecond * Constants::AudioBlockFrames / sampleRate)};
        // while (abs(diff) > kMaxDiff && initialReadIndex != packetBuffer.getReadIndex()) {
        while ((diff > kMaxDiff / 2 || diff < -kMaxDiff / 2) && initialReadIndex != playbackPacketBuffer.getReadIndex()) {
            // if (std::signbit(diff)) {
            //     packetBuffer.decrementReadIndex();
            // } else {
            playbackPacketBuffer.incrementReadIndex();
            // }
            packetTime = playbackPacketBuffer.peek().header.time;
            diff = now - packetTime;
            Serial.print("Current time: ");
            printTime(now);
            Serial.printf(", Read index: %" PRIu32 "\nPacket time:  ", playbackPacketBuffer.getReadIndex());
            printTime(packetTime);
            Serial.printf(", diff: %" PRId64 "\n", diff, kMaxDiff);
        }
        // sampleOffset = diff / static_cast<int64_t>(1e9 / sampleRate);
        // Serial.printf("Packet offset: %" PRId64 " ns, Sample offset: %" PRId32 "\n", diff, sampleOffset);
    }

    size_t AudioServer::printTo(Print &p) const
    {
        return 0;
    }
} // ananas
