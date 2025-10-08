#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <IPAddress.h>

#ifndef AUDIO_BLOCK_SAMPLES
#define AUDIO_BLOCK_SAMPLES  128
#endif

#ifndef NUM_SOURCES
#define NUM_SOURCES 16
#endif

namespace ananas
{
    struct Constants
    {
        static constexpr size_t AudioBlockFrames{AUDIO_BLOCK_SAMPLES};
        static constexpr size_t NumOutputChannels{2};
        static constexpr size_t MaxChannels{NUM_SOURCES};

        static constexpr int64_t NanosecondsPerSecond{1'000'000'000};
        static constexpr float NanosecondsPerCpuCycle{10.f / 6.f};

        inline static const IPAddress MulticastIP{224, 4, 224, 4};
        static constexpr uint16_t AudioPort{49152};

        static constexpr size_t SampleSizeBytes{sizeof(int16_t)};
        static constexpr size_t PacketBufferCapacity{50};
        static constexpr int64_t PacketReproductionOffsetNs{NanosecondsPerSecond / 20};
        static constexpr size_t FramesPerPacketExpected{32};

        static constexpr uint ClientReportThreshold{5000};
    };

    class Utils
    {
    public:
        static void hexDump(const uint8_t *buffer, const size_t length)
        {
            size_t word{0}, row{0};
            for (const uint8_t *p = buffer; word < length; ++p, ++word) {
                if (word % 16 == 0) {
                    if (word != 0) Serial.print(F("\n"));
                    Serial.printf(F("%04x: "), row);
                    ++row;
                } else if (word % 2 == 0) {
                    Serial.print(F(" "));
                }
                Serial.printf(F("%02x "), *p);
            }
            Serial.println(F("\n"));
        }

        static uint32_t cyclesToNs(const uint32_t numCycles)
        {
            return static_cast<uint32_t>(numCycles * Constants::NanosecondsPerCpuCycle);
        }
    };
}

#endif //UTILS_H
