#include "SerialClockMultiplexerRegister1.h"

SerialClockMultiplexerRegister1 SerialClockMultiplexerRegister1::instance_;

SerialClockMultiplexerRegister1 &SerialClockMultiplexerRegister1::instance()
{
    return instance_;
}

SerialClockMultiplexerRegister1 &SerialClockMultiplexerRegister1 = SerialClockMultiplexerRegister1::instance();

bool SerialClockMultiplexerRegister1::begin()
{
    // Unset SAI1 clock select
    write(CCM_CSCMR1 & ~CCM_CSCMR1_SAI1_CLK_SEL_MASK);
    return true;
}

bool SerialClockMultiplexerRegister1::setSai1ClkSel(const SerialClockMultiplexerRegister1::Sai1ClkSel selector)
{
    switch (selector) {
        case Sai1ClkSel::kReserved:
            Serial.printf("Reserved value provided for "
                          "CCM_CSCMR1_SAI1_CLK_SEL: %d\n",
                          selector);
            return false;
        default:
            write(getValue() | CCM_CSCMR1_SAI1_CLK_SEL((int) selector));
            return true;
    }
}
