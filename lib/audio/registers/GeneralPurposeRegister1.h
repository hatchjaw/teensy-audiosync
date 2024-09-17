#ifndef GENERALPURPOSEREGISTER1_H
#define GENERALPURPOSEREGISTER1_H

#include "IMXRT1060Register.h"

/**
 * GPR1 General Purpose Register (IOMUXC_GPR_GPR1), 0x4008C004
 * i.MX RT1060 Processor Reference Manual rev. 3, §11.3.2, p. 325
 * Implementation of audio-related fields
 * For testability, this class is not implemented as a singleton, but only one
 * instance of it should exist.
 */
class GeneralPurposeRegister1 final : public IMXRT1060Register
{
public:
    enum class SignalDirection : int
    {
        Input = 0,
        Output = 1
    };

    /**
     * An enumeration representing the available master clock sources for
     * SAI1_MCLK1/2.
     *
     * NB. some of these entries appear to be misspelled in rev. 3 of the manual
     * as "ssi[n]_clk_root"
     */
    enum class Sai1MclkSource : int
    {
        CcmSai1ClkRoot = 0,
        CcmSai2ClkRoot = 1,
        CcmSai3ClkRoot = 2,
        IomuxSai1IpgClkSaiMclk = 3,
        IomuxSai2IpgClkSaiMclk = 4,
        IomuxSai3IpgClkSaiMclk = 5,
        Reserved1 = 6,
        Reserved2 = 7
    };

    GeneralPurposeRegister1() :
        IMXRT1060Register("IOMUXC_GPR_GPR1", &IOMUXC_GPR_GPR1) {}

    bool begin() override;

    bool setSai1MclkDirection(SignalDirection direction) const;

    bool setSai1MclkSource(Sai1MclkSource source) const;
};


#endif //GENERALPURPOSEREGISTER1_H
