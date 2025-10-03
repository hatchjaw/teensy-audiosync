//
// Created by tar on 28/01/25.
//

#ifndef ANANASPACKET_H
#define ANANASPACKET_H

#include <lwip_t41.h>
#include <t41-ptp.h>
#include <AnanasUtils.h>

namespace ananas
{
    struct Packet
    {
        uint8_t *rawData() { return reinterpret_cast<uint8_t *>(this); }
        int16_t *audio() { return reinterpret_cast<int16_t *>(data); }
        NanoTime time;
        uint8_t data[Constants::AudioBlockFrames << 2]; // AUDIO_BLOCK_SAMPLES * NUM_CHANNELS * sizeof(int16_t)
    };

    struct PacketV2
    {
        struct alignas(16) Header
        {
            NanoTime time;
            uint8_t numChannels;
            uint16_t numFrames;
        };

        uint8_t *rawData() { return reinterpret_cast<uint8_t *>(this); }
        int16_t *audio() { return reinterpret_cast<int16_t *>(data); }
        Header header{};
        uint8_t data[MTU - sizeof(Header)]{};
    };
}

#endif //ANANASPACKET_H
