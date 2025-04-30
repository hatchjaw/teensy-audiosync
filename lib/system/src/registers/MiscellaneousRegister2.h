#ifndef MISCELLANEOUSREGISTER2_H
#define MISCELLANEOUSREGISTER2_H

#include "IMXRT1060Register.h"

/**
 * Miscellaneous Register 2 (CCM_ANALOG_MISC2n)
 * i.MX RT1060 Processor Reference Manual rev. 3, §14.8.19, p. 1126
 * Implementation of audio-related fields.
 * Only one instance of this class should exist.
 */
class MiscellaneousRegister2 final : public IMXRT1060BitbandRegister
{
public:
    enum class AudioPostDiv : int
    {
        DivideBy1 = 1,
        DivideBy2 = 2,
        DivideBy4 = 4
    };

    MiscellaneousRegister2()
        : IMXRT1060BitbandRegister("CCM_ANALOG_MISC2", &CCM_ANALOG_MISC2) {}

    bool begin() override;

    bool setAudioPostDiv(AudioPostDiv div) const;
};


#endif //MISCELLANEOUSREGISTER2_H
