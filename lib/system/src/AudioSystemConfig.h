#ifndef TEENSY_AUDIOSYNC_CONFIG_H
#define TEENSY_AUDIOSYNC_CONFIG_H

#include <Arduino.h>

struct ClockConstants
{
    /**
     * Clock cycles per second at 1 MHz
     */
    static constexpr uint32_t MHz{1'000'000};
    /**
     * Nominal frequency, in MHz, of the crystal oscillator of the iMX RT1060.
     *
     * F_X / 1×10^6 = 24 MHz
     */
    static constexpr uint32_t OscMHz{24};
    /**
     * Nominal frequency, in Hz, of the crystal oscillator of the iMX RT1060.
     *
     * F_X = 24×10^6 Hz
     */
    static constexpr uint32_t OscHz{OscMHz * MHz};
    /**
     * SAI1 word size.
     * The relationship between the audio sampling rate, F_s, and SAI1_CLK_ROOT,
     * F_i, is:
     *
     * F_s = F_i / 2^8.
     */
    static constexpr uint32_t AudioWordSize{1 << 8};
    /**
     * Ratio of the frequency of the crystal oscillator to the SAI1 word size;
     * used in various calculations. @see AudioSystemManager::calculateDividers.
     */
    static constexpr double CyclesPerWord{(double) OscHz / AudioWordSize};
    /**
     * Minimum valid value for CCM_ANALOG_PLL_AUDIO.DIV_SELECT (D_S).
     *
     * 27 <= D_S
     */
    static constexpr uint8_t Pll4DivMin{27};
    /**
     * Maximum valid value for CCM_ANALOG_PLL_AUDIO.DIV_SELECT (D_S).
     *
     * D_S <= 54
     */
    static constexpr uint8_t Pll4DivMax{54};
    /**
     * Maximum valid value for the CCM_ANALOG_PLL_AUDIO_DENOM register (D_D).
     * D_D is the "[unsigned] 30 bit denominator of [the] fractional loop divider"
     */
    static constexpr uint32_t Pll4DenomMax{(1 << 30) - 1};
    /**
     * Maximum valid value for the CCM_ANALOG_PLL_AUDIO_NUM register (D_N).
     * D_N is the "[signed] 30 bit numerator of [the] fractional loop divider".
     * Though described as a signed number, MCUXpresso Config Tools indicates
     * that D_N cannot take a negative value.
     */
    static constexpr int32_t Pll4NumMax{(1 << 29) - 1};
    /**
     * Minimum PLL4 output frequency.
     *
     * F_P = F_X * (D_S + D_N/D_D)
     *
     * 650 MHz < F_P.
     */
    static constexpr uint32_t Pll4FreqMin{650 * MHz};
    /**
     * Maximum PLL4 output frequency.
     *
     * F_P = F_X * (D_S + D_N/D_D)
     *
     * F_P < 1300 MHz.
     */
    static constexpr uint32_t Pll4FreqMax{1'300 * MHz};
    /**
     * Maximum valid value for the pre-divider field of the CCM Clock Divider
     * Register (CCM_CS1CDR.SAI1_CLK_PRED)
     */
    static constexpr uint8_t Sai1PreMax{8};
    /**
     * Maximum valid value for the post-divider field of the CCM Clock Divider
     * Register (CCM_CS1CDR.SAI1_CLK_PODF)
     */
    static constexpr uint8_t Sai1PostMax{64};
    /**
     * Maximum input frequency to the SAI1 post-divider.
     */
    static constexpr uint32_t Sai1PostMaxFreq{300 * MHz};
    /**
     * Nanoseconds per second.
     */
    static constexpr int64_t NanosecondsPerSecond{1'000'000'000};

    static constexpr double Nanosecond{1e-9};
};

struct AudioSystemConfig : Printable
{
    enum class ClockRole
    {
        Authority,
        Subscriber
    };

    AudioSystemConfig(const uint16_t bufferSize, const uint32_t samplingRate, const ClockRole clockRole, const float volume = .5f)
        : kSamplingRate(samplingRate),
          kBufferSize(bufferSize),
          kClockRole(clockRole),
          volume(volume),
          samplingRateExact(samplingRate)
    {
    }

    size_t printTo(Print &p) const override
    {
        return p.printf("%s - Frames/Fs: %" PRIu16 "/%" PRIu32 " (%.16f)",
                        kClockRole == ClockRole::Authority ? "Clock Authority" : "Clock Subscriber",
                        kBufferSize, kSamplingRate, samplingRateExact);
    }

    void setExactSamplingRate(const double proportionalAdjustment)
    {
        samplingRateExact = proportionalAdjustment * (double) kSamplingRate;
    }

    double getExactSamplingRate() const { return samplingRateExact; }

    const uint32_t kSamplingRate;
    const uint16_t kBufferSize;
    const ClockRole kClockRole;
    float volume;

private:
    double samplingRateExact;
};

#endif //TEENSY_AUDIOSYNC_CONFIG_H
