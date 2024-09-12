#ifndef INC_1588_MISCELLANEOUSREGISTER2_H
#define INC_1588_MISCELLANEOUSREGISTER2_H

#include <IMXRT1060Register.h>

/**
 * Miscellaneous Register 2 (CCM_ANALOG_MISC2n)
 * i.MX RT1060 Processor Reference Manual, §14.8.19, p. 1126
 */
class MiscellaneousRegister2 final : public IMXRT1060BitbandRegister
{
public:
    enum class AudioPostDiv : int
    {
        kDivideBy1 = 1,
        kDivideBy2 = 2,
        kDivideBy4 = 4
    };

    static MiscellaneousRegister2 &instance();

    bool begin() override;

    bool setAudioPostDiv(AudioPostDiv div);

private:
    MiscellaneousRegister2()
            : IMXRT1060BitbandRegister("CCM_ANALOG_MISC2", &CCM_ANALOG_MISC2) {}

    static MiscellaneousRegister2 instance_;
};


#endif //INC_1588_MISCELLANEOUSREGISTER2_H
