#include "MiscellaneousRegister2.h"

bool MiscellaneousRegister2::begin()
{
    setAudioPostDiv(AudioPostDiv::DivideBy1);
    return true;
}

bool MiscellaneousRegister2::setAudioPostDiv(const AudioPostDiv div) const
{
    switch (div) {
        case AudioPostDiv::DivideBy2:
            set(CCM_ANALOG_MISC2_AUDIO_DIV_LSB);
            break;
        case AudioPostDiv::DivideBy4:
            set(CCM_ANALOG_MISC2_AUDIO_DIV_LSB | CCM_ANALOG_MISC2_AUDIO_DIV_MSB);
            break;
        default:
            clear(CCM_ANALOG_MISC2_AUDIO_DIV_LSB | CCM_ANALOG_MISC2_AUDIO_DIV_MSB);
            break;
    }
    return true;
}
