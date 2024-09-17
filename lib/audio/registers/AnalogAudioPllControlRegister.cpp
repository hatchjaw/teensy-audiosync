#include "AnalogAudioPllControlRegister.h"
#include "ClockConstants.h"

bool AnalogAudioPllControlRegister::begin()
{
    return reset();
}

bool AnalogAudioPllControlRegister::setBypass(bool bypass) const
{
    if (bypass) {
        set(CCM_ANALOG_PLL_AUDIO_BYPASS);
    } else {
        clear(CCM_ANALOG_PLL_AUDIO_BYPASS);
    }
//    Serial.printf("bypass: CCM_ANALOG_PLL_AUDIO: %X\n", getValue());
    return true;
}

bool AnalogAudioPllControlRegister::setEnable(bool enable) const
{
    if (enable) {
        set(CCM_ANALOG_PLL_AUDIO_ENABLE);
    } else {
        clear(CCM_ANALOG_PLL_AUDIO_ENABLE);
    }
//    Serial.printf("enable: CCM_ANALOG_PLL_AUDIO: %X\n", getValue());
    return true;
}

bool AnalogAudioPllControlRegister::setPowerDown(bool powerDown) const
{
    if (powerDown) {
        set(CCM_ANALOG_PLL_AUDIO_POWERDOWN);
    } else {
        clear(CCM_ANALOG_PLL_AUDIO_POWERDOWN);
        awaitLock();
    }
//    Serial.printf("powerDown: CCM_ANALOG_PLL_AUDIO: %X\n", getValue());
    return true;
}

bool AnalogAudioPllControlRegister::setPostDivSelect(PostDivSelect postDivSelect) const
{
    switch (postDivSelect) {
        case PostDivSelect::Reserved:
            Serial.printf("Reserved value provided for "
                          "CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT: %d\n",
                          postDivSelect);
            return false;
        default:
            set(CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT((int) postDivSelect));
//            Serial.printf("setPostDivSelect: CCM_ANALOG_PLL_AUDIO: %X\n", getValue());
            return true;
    }
}

bool AnalogAudioPllControlRegister::setDivSelect(const int pll4Div) const
{
    if (pll4Div < 27 || pll4Div > 54) {
        Serial.printf("Invalid value provided for "
                      "CCM_ANALOG_PLL_AUDIO_DIV_SELECT: %d\n",
                      pll4Div);
        return false;
    }
    set(CCM_ANALOG_PLL_AUDIO_DIV_SELECT(pll4Div));
//    Serial.printf("setDivSelect: CCM_ANALOG_PLL_AUDIO: %X\n", getValue());
    return true;
}

bool AnalogAudioPllControlRegister::setBypassClockSource(const BypassClockSource source) const
{
    switch (source) {
        case BypassClockSource::Reserved1:
        case BypassClockSource::Reserved2:
            Serial.printf("Reserved value provided for "
                          "CCM_ANALOG_PLL_AUDIO_BYPASS_CLK_SRC: %d\n",
                          source);
            return false;
        default:
            set(CCM_ANALOG_PLL_AUDIO_BYPASS_CLK_SRC((int) source));
//            Serial.printf("setBypassClockSource: CCM_ANALOG_PLL_AUDIO: %X\n", getValue());
            return true;
    }
}

bool AnalogAudioPllControlRegister::reset() const
{
    write(CCM_ANALOG_PLL_AUDIO_POWERDOWN | CCM_ANALOG_PLL_AUDIO_BYPASS);
    return true;
}

void AnalogAudioPllControlRegister::awaitLock() const
{
    const auto cycles{ARM_DWT_CYCCNT};
    while (!(getValue() & CCM_ANALOG_PLL_AUDIO_LOCK)) {}
    Serial.printf("PLL4 lock took %" PRIu32 " cycles.\n", ARM_DWT_CYCCNT - cycles);
}

//==============================================================================

bool AudioPllNumeratorRegister::begin()
{
    return set(0);
}

bool AudioPllNumeratorRegister::set(const int32_t num) const
{
    if (num < 0 || num > ClockConstants::k_Pll4NumMax) {
        Serial.printf("Invalid value provided for "
                      "CCM_ANALOG_PLL_AUDIO_NUM: %" PRId32 "\n",
                      num);
        return false;
    }

    write((uint32_t) num);
    return true;
}

//==============================================================================

bool AudioPllDenominatorRegister::begin()
{
    return set(1);
}

bool AudioPllDenominatorRegister::set(const uint32_t denom) const
{
    if (denom == 0 || denom > ClockConstants::k_Pll4DenomMax) {
        Serial.printf("Invalid value provided for "
                      "CCM_ANALOG_PLL_AUDIO_DENOM: %" PRId32 "\n",
                      denom);
        return false;
    }

    write(denom);
    return true;
}
