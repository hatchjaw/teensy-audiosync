#include "MiscellaneousRegister2.h"

MiscellaneousRegister2 MiscellaneousRegister2::instance_;

MiscellaneousRegister2 &MiscellaneousRegister2::instance()
{
    return instance_;
}

MiscellaneousRegister2 &MiscellaneousRegister2 = MiscellaneousRegister2::instance();

bool MiscellaneousRegister2::begin()
{
    setAudioPostDiv(AudioPostDiv::kDivideBy1);
    return false;
}

bool MiscellaneousRegister2::setAudioPostDiv(const AudioPostDiv div)
{
    switch (div) {
        case AudioPostDiv::kDivideBy2:
            set(CCM_ANALOG_MISC2_AUDIO_DIV_LSB);
            break;
        case AudioPostDiv::kDivideBy4:
            set(CCM_ANALOG_MISC2_AUDIO_DIV_LSB | CCM_ANALOG_MISC2_AUDIO_DIV_MSB);
            break;
        default:
            clear(CCM_ANALOG_MISC2_AUDIO_DIV_LSB | CCM_ANALOG_MISC2_AUDIO_DIV_MSB);
            break;
    }
    return true;
}
