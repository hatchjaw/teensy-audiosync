#include "GeneralPurposeRegister1.h"

bool GeneralPurposeRegister1::begin()
{
    write(getValue() & ~IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK & ~IOMUXC_GPR_GPR1_SAI1_MCLK_DIR);
    return true;
}

bool GeneralPurposeRegister1::setSai1MclkDirection(const SignalDirection direction) const
{
    writeMask(direction == SignalDirection::Output, IOMUXC_GPR_GPR1_SAI1_MCLK_DIR);
    return true;
}

bool GeneralPurposeRegister1::setSai1MclkSource(const Sai1MclkSource source) const
{
    switch (source) {
        case Sai1MclkSource::Reserved1:
        case Sai1MclkSource::Reserved2:
            Serial.printf("Reserved value provided for "
                          "IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL: %d\n",
                          source);
            return false;
        default:
            write(getValue() | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL((int) source));
            return true;
    }
}
