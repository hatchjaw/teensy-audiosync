//
// Created by tar on 28/01/25.
//

#ifndef ANANASPACKET_H
#define ANANASPACKET_H

#include <lwip_t41.h>
#include <Utils.h>

namespace ananas
{
    struct Packet
    {
        uint8_t *rawData() { return reinterpret_cast<uint8_t *>(this); }
        int16_t *audio() { return reinterpret_cast<int16_t *>(data); }
        NanoTime time;
        uint8_t data[Constants::kAudioBlockFrames << 2]; // AUDIO_BLOCK_SAMPLES * NUM_CHANNELS * sizeof(int16_t)
    };

    struct PacketV2
    {
        uint8_t *rawData() { return reinterpret_cast<uint8_t *>(this); }
        int16_t *audio() { return reinterpret_cast<int16_t *>(data); }
        NanoTime time;
        uint8_t numChannels;
        uint8_t numFrames;
        uint8_t data[MTU - sizeof(numFrames) - sizeof(numChannels) - sizeof(time)];
    };
}

#endif //ANANASPACKET_H
