#include "GeneralPurposeRegister1.h"

GeneralPurposeRegister1 GeneralPurposeRegister1::instance_;

GeneralPurposeRegister1 &GeneralPurposeRegister1::instance()
{
    return instance_;
}

GeneralPurposeRegister1 &GeneralPurposeRegister1 = GeneralPurposeRegister1::instance();

bool GeneralPurposeRegister1::begin()
{
    write(getValue() & ~IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK & ~IOMUXC_GPR_GPR1_SAI1_MCLK_DIR);
    return true;
}

bool GeneralPurposeRegister1::setSai1MclkDirection(const SignalDirection direction) const
{
    switch (direction) {
        case SignalDirection::kInput:
            write(getValue() & ~IOMUXC_GPR_GPR1_SAI1_MCLK_DIR);
            break;
        case SignalDirection::kOutput:
            write(getValue() | IOMUXC_GPR_GPR1_SAI1_MCLK_DIR);
            break;
    }
    return true;
}

bool GeneralPurposeRegister1::setSai1MclkSource(const Sai1MclkSource source) const
{
    switch (source) {
        case Sai1MclkSource::kCcmSsi1ClkRoot:
        case Sai1MclkSource::kCcmSsi2ClkRoot:
        case Sai1MclkSource::kCcmSsi3ClkRoot:
        case Sai1MclkSource::kIomuxSai1IpgClkSaiMclk:
        case Sai1MclkSource::kIomuxSai2IpgClkSaiMclk:
        case Sai1MclkSource::kIomuxSai3IpgClkSaiMclk:
            write(getValue() | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL((int) source));
            return true;
        case Sai1MclkSource::kReserved1:
        case Sai1MclkSource::kReserved2:
            Serial.printf("Reserved value provided for "
                          "IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL: %d\n",
                          source);
            return false;
    }
}
