#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <IPAddress.h>

#ifndef AUDIO_SAMPLE_RATE_EXACT
#define AUDIO_SAMPLE_RATE_EXACT 48000
#endif

#ifndef AUDIO_BLOCK_SAMPLES
#define AUDIO_BLOCK_SAMPLES  32
#endif

#ifndef NUM_SOURCES
#define NUM_SOURCES 16
#endif

#define CYCLES_TO_APPROX_PERCENT(cycles) (((float)((uint32_t)(cycles) * 6400u) * (float)(AUDIO_SAMPLE_RATE_EXACT / AUDIO_BLOCK_SAMPLES)) / (float)(F_CPU_ACTUAL))

namespace ananas
{
    struct Constants
    {
        static constexpr size_t AudioBlockFrames{AUDIO_BLOCK_SAMPLES};
        static constexpr uint32_t AudioSamplingRate{AUDIO_SAMPLE_RATE_EXACT};
        static constexpr size_t NumOutputChannels{2};
        static constexpr size_t MaxChannels{NUM_SOURCES};

        static constexpr int64_t NanosecondsPerSecond{1'000'000'000};
        static constexpr float NanosecondsPerCpuCycle{10.f / 6.f};

        inline static const IPAddress AudioMulticastIP{224, 4, 224, 4};
        inline static const IPAddress ControlMulticastIP{224, 4, 224, 5};
        inline static const IPAddress ClientAnnounceMulticastIP{224, 4, 224, 6};
        inline static const IPAddress AuthorityAnnounceMulticastIP{224, 4, 224, 7};
        inline static const IPAddress RebootMulticastIP{224, 4, 224, 8};
        inline static const IPAddress WFSControlMulticastIP{224, 4, 224, 10};
        static constexpr uint16_t AudioPort{49152};
        static constexpr uint16_t ClientAnnouncePort{49153};
        static constexpr uint16_t AuthorityAnnouncePort{49154};
        static constexpr uint16_t RebootPort{49155};
        static constexpr uint16_t WFSControlPort{49160};

        static constexpr size_t SampleSizeBytes{sizeof(int16_t)};
        static constexpr size_t PacketBufferCapacity{50};
        static constexpr int64_t PacketReproductionOffsetNs{NanosecondsPerSecond / 20};
        static constexpr size_t FramesPerPacketExpected{32};

        static constexpr uint ClientReportThresholdPackets{10000};
        static constexpr uint ClientAnnounceIntervalMs{500};

        static constexpr uint AuthorityAnnounceIntervalMs{500};
        static constexpr uint GroupEvaluationIntervalMs{1000};
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

        static float clamp(float value, const float min, const float max)
        {
            if (value < min) {
                value = min;
            } else if (value > max) {
                value = max;
            }

            return value;
        }
    };

    template<typename T>
    struct ListenableParameter {
        ListenableParameter() = default;

        explicit ListenableParameter(T v) : value(v) {};

        std::function<void(T val)> onChange;

        ListenableParameter &operator=(T newValue) {
            if (value != newValue) {
                value = newValue;
                if (onChange != nullptr) {
                    onChange(value);
                }
            }
            return *this;
        };

    private:
        T value;
    };

    /**
     * A low-pass filter to smooth parameter changes. See Faust's si.smooth.
     * @tparam T
     */
    template<typename T>
    class SmoothedValue {
    public:
        explicit SmoothedValue(T initialValue, float smoothness, T threshold = 1e-6) :
                x(initialValue),
                yPrev(initialValue),
                deltaThreshold(threshold),
                s(Utils::clamp(smoothness, 0.f, 1.f)) {}

        void set(T targetValue) {
            x = targetValue;

            if (onSet != nullptr) {
                onSet(x);
            }
        }

        T getNext() {
            T y;
            if (abs(yPrev - x) < deltaThreshold) {
                yPrev = x;
                y = yPrev;
            } else {
                // 1-pole lowpass: y[n] = (1 - s) * x[n] + s * y[n - 1]
                auto ts{static_cast<T>(s)};
                y = (1.f - ts) * x + ts * yPrev;
                // Serial.printf("y = %.9f; yPrev = %.9f\n", y, yPrev);
                yPrev = y;

                if (onChange != nullptr) {
                    onChange(y);
                }
            }

            return y;
        }

        std::function<void(T newValue)> onSet;
        std::function<void(T currentValue)> onChange;

    private:
        T x, yPrev, deltaThreshold;
        float s;
    };
}

#endif //UTILS_H
