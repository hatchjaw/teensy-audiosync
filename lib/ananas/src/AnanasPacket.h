#ifndef ANANASPACKET_H
#define ANANASPACKET_H

#include "AnanasUtils.h"
#include <lwip_t41.h>
#include <t41-ptp.h>

namespace ananas
{
    struct Packet
    {
        uint8_t *rawData() { return reinterpret_cast<uint8_t *>(this); }
    };

    struct AudioPacket : Packet
    {
        struct alignas(16) Header
        {
            NanoTime time;
            uint8_t numChannels;
            uint16_t numFrames;
        };

        int16_t *audio() { return reinterpret_cast<int16_t *>(data); }

        Header header{};
        uint8_t data[MTU - sizeof(Header)]{};
    };

    struct ClientAnnouncePacket : Packet
    {
        uint32_t serial;
        float samplingRate;
        float percentCPU;
        int32_t presentationOffsetFrame;
        NanoTime presentationOffsetNs;
        int32_t audioPtpOffsetNs;
        uint8_t bufferFillPercent;
        bool ptpLock;
        uint16_t moduleID;
    };

    struct AuthorityAnnouncePacket : Packet
    {
        uint32_t serial;
        uint32_t usbFeedbackAccumulator;
        int numClients;
        int avgBufferFillPercent;
        int numUnderruns;
        int numOverflows;
    };
}

#endif //ANANASPACKET_H
