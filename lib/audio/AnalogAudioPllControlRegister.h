#ifndef INC_1588_ANALOGAUDIOPLLCONTROLREGISTER_H
#define INC_1588_ANALOGAUDIOPLLCONTROLREGISTER_H

#include <IMXRT1060Register.h>

/**
 * Analog Audio PLL control Register (CCM_ANALOG_PLL_AUDIOn)
 * i.MX RT1060 Processor Reference Manual, §14.8.8, p. 1104
 */
class AnalogAudioPllControlRegister final : public IMXRT1060BitbandRegister
{
public:
    enum class PostDivSelect : int
    {
        kDivideBy4 = 0,
        kDivideBy2 = 1,
        kDivideBy1 = 2,
        kReserved = 3
    };

    enum class BypassClockSource : int
    {
        kRefClk24M = 0,
        kClk1 = 1,
        kReserved1 = 2,
        kReserved2 = 3
    };

    static AnalogAudioPllControlRegister &instance();

    bool begin() override;

    bool setBypass(bool bypass);

    bool setEnable(bool enable);

    bool setPowerDown(bool powerDown);

    bool setBypassClockSource(AnalogAudioPllControlRegister::BypassClockSource source);

    bool setPostDivSelect(PostDivSelect postDivSelect);

    bool setDivSelect(int pll4Div);

private:
    AnalogAudioPllControlRegister()
            : IMXRT1060BitbandRegister("CCM_ANALOG_PLL_AUDIO", &CCM_ANALOG_PLL_AUDIO) {}

    bool reset();

    static AnalogAudioPllControlRegister instance_;

    void awaitLock();
};

//==============================================================================

/**
 * Numerator of Audio PLL Fractional Loop Divider Register
 * (CCM_ANALOG_PLL_AUDIO_NUM)
 * i.MX RT1060 Processor Reference Manual, §14.8.9, p. 1106
 */
class AudioPllNumeratorRegister final : public IMXRT1060Register
{
public:
    static AudioPllNumeratorRegister &instance();

    bool begin() override;

    bool set(int32_t num);

private:
    AudioPllNumeratorRegister()
            : IMXRT1060Register("CCM_ANALOG_PLL_AUDIO_NUM", &CCM_ANALOG_PLL_AUDIO_NUM) {}

    static AudioPllNumeratorRegister instance_;
};

//==============================================================================

/**
 * Denominator of Audio PLL Fractional Loop Divider Register
 * (CCM_ANALOG_PLL_AUDIO_DENOM)
 * i.MX RT1060 Processor Reference Manual, §14.8.10, p. 1106
 */
class AudioPllDenominatorRegister final : public IMXRT1060Register
{
public:
    static AudioPllDenominatorRegister &instance();

    bool begin() override;

    bool set(uint32_t denom);

private:
    AudioPllDenominatorRegister()
            : IMXRT1060Register("CCM_ANALOG_PLL_AUDIO_DENOM", &CCM_ANALOG_PLL_AUDIO_DENOM) {}

    static AudioPllDenominatorRegister instance_;
};

#endif //INC_1588_ANALOGAUDIOPLLCONTROLREGISTER_H
