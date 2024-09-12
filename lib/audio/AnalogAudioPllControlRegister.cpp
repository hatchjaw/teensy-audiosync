#include "AnalogAudioPllControlRegister.h"
#include <ClockConstants.h>

AnalogAudioPllControlRegister AnalogAudioPllControlRegister::instance_;

AnalogAudioPllControlRegister &AnalogAudioPllControlRegister::instance()
{
    return instance_;
}

AnalogAudioPllControlRegister &AnalogAudioPllControlRegister = AnalogAudioPllControlRegister::instance();

bool AnalogAudioPllControlRegister::begin()
{
    return reset();
}

bool AnalogAudioPllControlRegister::setBypass(bool bypass)
{
    if (bypass) {
        set(CCM_ANALOG_PLL_AUDIO_BYPASS);
    } else {
        clear(CCM_ANALOG_PLL_AUDIO_BYPASS);
    }
//    Serial.printf("bypass: CCM_ANALOG_PLL_AUDIO: %X\n", getValue());
    return true;
}

bool AnalogAudioPllControlRegister::setEnable(bool enable)
{
    if (enable) {
        set(CCM_ANALOG_PLL_AUDIO_ENABLE);
    } else {
        clear(CCM_ANALOG_PLL_AUDIO_ENABLE);
    }
//    Serial.printf("enable: CCM_ANALOG_PLL_AUDIO: %X\n", getValue());
    return true;
}

bool AnalogAudioPllControlRegister::setPowerDown(bool powerDown)
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

bool AnalogAudioPllControlRegister::setPostDivSelect(AnalogAudioPllControlRegister::PostDivSelect postDivSelect)
{
    switch (postDivSelect) {
        case PostDivSelect::kReserved:
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

bool AnalogAudioPllControlRegister::setDivSelect(int pll4Div)
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

bool AnalogAudioPllControlRegister::setBypassClockSource(AnalogAudioPllControlRegister::BypassClockSource source)
{
    switch (source) {
        case BypassClockSource::kReserved1:
        case BypassClockSource::kReserved2:
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

bool AnalogAudioPllControlRegister::reset()
{
    write(CCM_ANALOG_PLL_AUDIO_POWERDOWN | CCM_ANALOG_PLL_AUDIO_BYPASS);
    return true;
}

void AnalogAudioPllControlRegister::awaitLock()
{
    auto cycles{ARM_DWT_CYCCNT};
    while (!(getValue() & CCM_ANALOG_PLL_AUDIO_LOCK)) {}
    Serial.printf("PLL4 lock took %" PRIu32 " cycles.\n", ARM_DWT_CYCCNT - cycles);
}

//==============================================================================

AudioPllNumeratorRegister AudioPllNumeratorRegister::instance_;

AudioPllNumeratorRegister &AudioPllNumeratorRegister::instance()
{
    return instance_;
}

AudioPllNumeratorRegister &AudioPllNumeratorRegister = AudioPllNumeratorRegister::instance();

bool AudioPllNumeratorRegister::begin()
{
    return set(0);
}

bool AudioPllNumeratorRegister::set(int32_t num)
{
    if (num < 0 || num > ClockConstants::k_pll4NumMax) {
        Serial.printf("Invalid value provided for "
                      "CCM_ANALOG_PLL_AUDIO_NUM: %" PRId32 "\n",
                      num);
        return false;
    }

    write((uint32_t) num);
    return true;
}

//==============================================================================

AudioPllDenominatorRegister AudioPllDenominatorRegister::instance_;

AudioPllDenominatorRegister &AudioPllDenominatorRegister::instance()
{
    return instance_;
}

AudioPllDenominatorRegister &AudioPllDenominatorRegister = AudioPllDenominatorRegister::instance();

bool AudioPllDenominatorRegister::begin()
{
    return set(1);
}

bool AudioPllDenominatorRegister::set(uint32_t denom)
{
    if (denom == 0 || denom > ClockConstants::k_pll4DenomMax) {
        Serial.printf("Invalid value provided for "
                      "CCM_ANALOG_PLL_AUDIO_DENOM: %" PRId32 "\n",
                      denom);
        return false;
    }

    write(denom);
    return true;
}
