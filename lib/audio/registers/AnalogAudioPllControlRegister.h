#ifndef ANALOGAUDIOPLLCONTROLREGISTER_H
#define ANALOGAUDIOPLLCONTROLREGISTER_H

#include "IMXRT1060Register.h"

/**
 * Analog Audio PLL control Register (CCM_ANALOG_PLL_AUDIOn)
 * i.MX RT1060 Processor Reference Manual rev. 3, §14.8.8, p. 1104
 * For testability, this class is not implemented as a singleton, but only one
 * instance of it should exist.
 */
class AnalogAudioPllControlRegister final : public IMXRT1060BitbandRegister
{
public:
    enum class PostDivSelect : int
    {
        DivideBy4 = 0,
        DivideBy2 = 1,
        DivideBy1 = 2,
        Reserved = 3
    };

    enum class BypassClockSource : int
    {
        RefClk24M = 0,
        Clk1 = 1,
        Reserved1 = 2,
        Reserved2 = 3
    };

    AnalogAudioPllControlRegister()
        : IMXRT1060BitbandRegister("CCM_ANALOG_PLL_AUDIO", &CCM_ANALOG_PLL_AUDIO)
    {
    }

    bool begin() override;

    /**
     * Set the BYPASS field of the CCM_ANALOG_PLL_AUDIO register.
     * @param bypass <b>true</b> to bypass the PLL4 fractional divider and
     * post divider; <b>false</b> to apply the dividers to the 24 MHz clock
     * source.
     * @return
     */
    bool setBypass(bool bypass) const;

    /**
     * Set the ENABLE field of the CCM_ANALOG_PLL_AUDIO register.
     * @param enable <b>true</b> to enable PLL4 clock output; <b>false</b> to
     * disable.
     * @return
     */
    bool setEnable(bool enable) const;

    /**
     * Set the POWERDOWN field of the CCM_ANALOG_PLL_AUDIO register.
     * @param powerDown <b>true</b> to power down the PLL4 clock; <b>false</b>
     * to power up.
     * @return
     */
    bool setPowerDown(bool powerDown) const;

    /**
     * Set the BYPASS_CLK_SRC field of the CCM_ANALOG_PLL_AUDIO register.
     * Choose between the internal 24 MHz reference clock, or an external clock
     * source.
     * @param source The clock source to use for PLL4.
     * @return true on success; false otherwise.
     */
    bool setBypassClockSource(BypassClockSource source) const;

    /**
     * Set the POST_DIV_SELECT field of the CCM_ANALOG_PLL_AUDIO register. PLL4
     * can be divided by 1, 2 or 4 after the fractional divider.
     * @param postDivSelect The divider to use.
     * @return true on success; false otherwise.
     */
    bool setPostDivSelect(PostDivSelect postDivSelect) const;

    /**
     * Set the DIV_SELECT field of the CCM_ANALOG_PLL_AUDIO register. Valid
     * range is 27 <= DIV_SELECT <= 54. NB. PLL4 output frequency must be in
     * range 650 MHz <= F_P <= 1300 MHz, where:
     * F_P = 24 MHz * (DIV_SELECT + CCM_ANALOG_PLL_AUDIO_NUM / CCM_ANALOG_PLL_AUDIO_DENOM)
     * @see AudioPllNumeratorRegister, AudioPllDenominatorRegister
     * @param pll4Div The divider to use.
     * @return true on success; false otherwise.
     */
    bool setDivSelect(int pll4Div) const;

private:
    /**
     * Reset the CCM_ANALOG_PLL_AUDIO register; powers down PLL4 and bypasses it.
     * @return
     */
    bool reset() const;

    /**
     * Wait for PLL4 to lock. Important when powering up.
     */
    void awaitLock() const;
};

//==============================================================================

/**
 * Numerator of Audio PLL Fractional Loop Divider Register
 * (CCM_ANALOG_PLL_AUDIO_NUM)
 * i.MX RT1060 Processor Reference Manual rev. 3, §14.8.9, p. 1106
 */
class AudioPllNumeratorRegister final : public IMXRT1060Register
{
public:
    AudioPllNumeratorRegister()
        : IMXRT1060Register("CCM_ANALOG_PLL_AUDIO_NUM", &CCM_ANALOG_PLL_AUDIO_NUM)
    {
    }

    bool begin() override;

    /**
     * Set the value of the numerator register of the PLL4 fractional loop
     * divider. Must be less than the value provided to the denominator register
     * and less than 2^{29}.
     *
     * The i.MX RT1060 manual suggests that a negative numerator may be
     * provided, but MCUXpresso config tools begs to differ.
     *
     * @param num The numerator value to use.
     * @return true on success; false otherwise.
     */
    bool set(int32_t num) const;
};

//==============================================================================

/**
 * Denominator of Audio PLL Fractional Loop Divider Register
 * (CCM_ANALOG_PLL_AUDIO_DENOM)
 * i.MX RT1060 Processor Reference Manual rev. 3, §14.8.10, p. 1106
 */
class AudioPllDenominatorRegister final : public IMXRT1060Register
{
public:
    AudioPllDenominatorRegister()
        : IMXRT1060Register("CCM_ANALOG_PLL_AUDIO_DENOM", &CCM_ANALOG_PLL_AUDIO_DENOM)
    {
    }

    bool begin() override;

    /**
     * Set the value of the denominator register of the PLL4 fractional loop
     * divider. Must be greater than the value provided to the numerator
     * register, and less than 2^{30}.
     * @param denom The value to use for the denominator.
     * @return true on success; false otherwise.
     */
    bool set(uint32_t denom) const;
};

#endif //ANALOGAUDIOPLLCONTROLREGISTER_H
