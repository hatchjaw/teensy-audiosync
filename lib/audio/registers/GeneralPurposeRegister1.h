#ifndef GENERALPURPOSEREGISTER1_H
#define GENERALPURPOSEREGISTER1_H

#include "IMXRT1060Register.h"

/**
 * GPR1 General Purpose Register (IOMUXC_GPR_GPR1), 0x4008C004
 * i.MX RT1060 Processor Reference Manual v3, §11.3.2, p. 325
 * Implementation of audio-related fields
 */
class GeneralPurposeRegister1 final : public IMXRT1060Register
{
public:
    enum class SignalDirection : int
    {
        kInput = 0,
        kOutput = 1
    };

    enum class Sai1MclkSource : int
    {
        kCcmSsi1ClkRoot = 0,
        kCcmSsi2ClkRoot = 1,
        kCcmSsi3ClkRoot = 2,
        kIomuxSai1IpgClkSaiMclk = 3,
        kIomuxSai2IpgClkSaiMclk = 4,
        kIomuxSai3IpgClkSaiMclk = 5,
        kReserved1 = 6,
        kReserved2 = 7
    };

    static GeneralPurposeRegister1 &instance();

    bool begin() override;

    bool setSai1MclkDirection(SignalDirection direction) const;

    bool setSai1MclkSource(Sai1MclkSource source) const;

private:
    GeneralPurposeRegister1() :
            IMXRT1060Register("IOMUXC_GPR_GPR1", &IOMUXC_GPR_GPR1) {}

    static GeneralPurposeRegister1 instance_;
};


#endif //GENERALPURPOSEREGISTER1_H
