#include "AnanasServer.h"

#include <AnanasPacket.h>
#include <NetAudioServer.h>
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
    }

    void AudioServer::processAudio(int16_t *buffer, const size_t numChannels, const size_t numSamples)
    {
        if (!connected) return;

        // Write incoming buffer to a packet in the packet buffer.
        memcpy(txPacket.data, buffer, numSamples * numChannels * sizeof(int16_t));

        // Get the current ethernet time.
        timespec ts{};
        EthernetIEEE1588.readTimer(ts);
        const NanoTime now{ts.tv_sec * NS_PER_S + ts.tv_nsec};
        // Set packet time some way in the future.
        txPacket.time = now + kPacketReproductionOffsetNs;

        packetBuffer.write(txPacket);
    }

    void AudioServer::send()
    {
        if (packetBuffer.isFull()) return;

        socket.beginPacket({224, 4, 224, 4}, 49152);
        socket.write(packetBuffer.read().rawData(), sizeof(Packet));
        socket.endPacket();
    }
} // ananas
