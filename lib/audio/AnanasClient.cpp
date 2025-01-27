#include "AnanasClient.h"
#include <QNEthernet.h>
#include "Utils.h"

ananas::AudioClient::AudioClient()
    : rxPacket(std::make_unique<Packet>())
{
}

void ananas::AudioClient::begin()
{
}

static NanoTime prevTime{0};
static NanoTime totalTime{0};

void ananas::AudioClient::loop()
{
    if (const auto size{socket.parsePacket()}; size > 0) {
        if (size == kPacketSize) {
            nWrite++;
            socket.read(rxPacket->rawData(), size);
            packetBuffer.write(rxPacket);

            // if (nWrite % 1000 == 0) {
            //     Serial.println("Received packet:");
            //     Utils::hexDump(rxPacket->rawData(), size);
            // }

            if (nWrite > 5000) {
                timespec now{};
                qindesign::network::EthernetIEEE1588.readTimer(now);
                const auto ns{now.tv_sec * 1'000'000'000 + now.tv_nsec};

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

void ananas::AudioClient::connect()
{
    socket.beginMulticast({224, 4, 224, 4}, 49152);
}

// static int16_t s{-(1 << 15)};

void ananas::AudioClient::processAudio(int16_t *dest, const size_t numChannels, const size_t numSamples)
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

    memcpy(dest, audioData, sizeof(int16_t) * numChannels * numSamples);

    // for (auto n{0}; n < numSamples; ++n) {
    //     for (auto c{0}; c < numChannels; ++c) {
    //         dest[n * numChannels + c] = s;
    //     }
    //     s += 1 << 8;
    // }
}

void ananas::AudioClient::printStats() const
{
    Serial.print(packetBuffer);
    Serial.printf("Packets received: %" PRIu16 " Average reception interval: %e ns\n",
                  nWrite,
                  static_cast<double>(totalTime) / (nWrite - 5000));
}

void ananas::AudioClient::adjustBufferReadIndex(NanoTime now)
{
    const auto idx{packetBuffer.getReadIndex()},
            initialReadIndex{idx == 0 ? PacketBuffer::kPacketBufferSize - 1 : idx - 1};
    auto packetTime{packetBuffer.peek().time};
    auto diff{now - packetTime};
    while (abs(diff) > 2'666'666 && initialReadIndex != packetBuffer.getReadIndex()) {
        if (std::signbit(diff)) {
            packetBuffer.decrementReadIndex();
        } else {
            packetBuffer.incrementReadIndex();
        }
        packetTime = packetBuffer.peek().time;
        diff = now - packetTime;
        Serial.print("Current time: ");
        Utils::printTime(now);
        Serial.printf(", Read index: %" PRIu32 "\nPacket time:  ", packetBuffer.getReadIndex());
        Utils::printTime(packetTime);
        Serial.printf(", diff: %" PRId64 "\n", diff);
    }
}

void ananas::AudioClient::PacketBuffer::write(const std::unique_ptr<Packet> &packet)
{
    // memcpy(&packetBuffer[writeIndex], packet->rawData(), sizeof(NanoTime) + (128 << 2));
    packetBuffer[writeIndex] = *packet;
    writeIndex = (writeIndex + 1) % kPacketBufferSize;
    // Might be better to avoid a division...
    // ++writeIndex;
    // if (writeIndex >= kPacketBufferSize) {
    // writeIndex = 0;
    // }
}

ananas::AudioClient::Packet &ananas::AudioClient::PacketBuffer::read()
{
    auto &packet{peek()};
    readIndex = (readIndex + 1) % kPacketBufferSize;
    // Might be better to avoid a division...
    // ++readIndex;
    // if (readIndex >= kPacketBufferSize) {
    // readIndex = 0;
    // }
    return packet;
}

ananas::AudioClient::Packet &ananas::AudioClient::PacketBuffer::peek()
{
    return packetBuffer[readIndex];
}

size_t ananas::AudioClient::PacketBuffer::printTo(Print &p) const
{
    return p.printf("Read index: %" PRIu32 ", Write index: %" PRIu32 ", Num packets available: %" PRIu32 "\n",
                    readIndex,
                    writeIndex,
                    writeIndex > readIndex ? writeIndex - readIndex : writeIndex + kPacketBufferSize - readIndex);
}

void ananas::AudioClient::PacketBuffer::incrementReadIndex()
{
    readIndex = (readIndex + 1) % kPacketBufferSize;
}

void ananas::AudioClient::PacketBuffer::decrementReadIndex()
{
    readIndex = (readIndex + kPacketBufferSize - 1) % kPacketBufferSize;
}

size_t ananas::AudioClient::PacketBuffer::getReadIndex() const
{
    return readIndex;
}
