#ifndef CLOCKCONSTANTS_H
#define CLOCKCONSTANTS_H

#include <Arduino.h>

struct ClockConstants
{
    /**
     * Clock cycles per second at 1 MHz
     */
    static constexpr uint32_t k_MHz{1'000'000};
    /**
     * Nominal frequency, in MHz, of the crystal oscillator of the iMX RT1060.
     * F_X / 10e6 = 24
     */
    static constexpr uint32_t k_OscMHz{24};
    /**
     * Nominal frequency, in Hz, of the crystal oscillator of the iMX RT1060.
     * F_X = 24e6
     */
    static constexpr uint32_t k_OscHz{k_OscMHz * k_MHz};
    /**
     * SAI1 word size.
     * The relationship between the audio sampling rate, F_s, and SAI1_CLK_ROOT,
     * F_i, is: F_s = F_i / 2^8.
     */
    static constexpr uint32_t k_AudioWordSize{1 << 8};
    /**
     * Ratio of the frequency of the crystal oscillator to the SAI1 word size;
     * used in various calculations. @see AudioSystemManager::calculateDividers.
     */
    static constexpr double k_CyclesPerWord{(double) k_OscHz / k_AudioWordSize};
    /**
     * Minimum valid value for CCM_ANALOG_PLL_AUDIO.DIV_SELECT (D_S).
     * 27 \<= D_S
     */
    static constexpr uint8_t k_Pll4DivMin{27};
    /**
     * Maximum valid value for CCM_ANALOG_PLL_AUDIO.DIV_SELECT (D_S).
     * D_S \<= 54
     */
    static constexpr uint8_t k_Pll4DivMax{54};
    /**
     * Maximum valid value for the CCM_ANALOG_PLL_AUDIO_DENOM register (D_D).
     * D_D is the "[unsigned] 30 bit denominator of [the] fractional loop divider"
     */
    static constexpr uint32_t k_Pll4DenomMax{(1 << 30) - 1};
    /**
     * Maximum valid value for the CCM_ANALOG_PLL_AUDIO_NUM register (D_N).
     * D_N is the "[signed] 30 bit numerator of [the] fractional loop divider".
     * Though described as a signed number, MCUXpresso Config Tools indicates
     * that D_N cannot take a negative value.
     */
    static constexpr int32_t k_Pll4NumMax{(1 << 29) - 1};
    /**
     * Minimum PLL4 output frequency.\n
     * F_P = F_X * (D_S + D_N/D_D)\n
     * 650 MHz \< F_P.
     */
    static constexpr uint32_t k_Pll4FreqMin{650 * k_MHz};
    /**
     * Maximum PLL4 output frequency.\n
     * F_P = F_X * (D_S + D_N/D_D)\n
     * F_P \< 1300 MHz.
     */
    static constexpr uint32_t k_Pll4FreqMax{1'300 * k_MHz};
    /**
     * Maximum valid value for the pre-divider field of the CCM Clock Divider
     * Register (CCM_CS1CDR.SAI1_CLK_PRED)
     */
    static constexpr uint8_t k_Sai1PreMax{8};
    /**
     * Maximum valid value for the post-divider field of the CCM Clock Divider
     * Register (CCM_CS1CDR.SAI1_CLK_PODF)
     */
    static constexpr uint8_t k_Sai1PostMax{64};
    /**
     * Maximum input frequency to the SAI1 post-divider.
     */
    static constexpr uint32_t k_Sai1PostMaxFreq{300 * k_MHz};
};

#endif //CLOCKCONSTANTS_H
