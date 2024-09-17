#include "SerialClockMultiplexerRegister1.h"

SerialClockMultiplexerRegister1 SerialClockMultiplexerRegister1::s_Instance;

SerialClockMultiplexerRegister1 &SerialClockMultiplexerRegister1::instance()
{
    return s_Instance;
}

SerialClockMultiplexerRegister1 &SerialClockMultiplexerRegister1 = SerialClockMultiplexerRegister1::instance();

bool SerialClockMultiplexerRegister1::begin()
{
    // Unset SAI1 clock select
    write(getValue() & ~CCM_CSCMR1_SAI1_CLK_SEL_MASK);
    return true;
}

bool SerialClockMultiplexerRegister1::setSai1ClkSel(const Sai1ClkSel selector) const
{
    switch (selector) {
        case Sai1ClkSel::Reserved:
            Serial.printf("Reserved value provided for "
                          "CCM_CSCMR1_SAI1_CLK_SEL: %d\n",
                          selector);
            return false;
        default:
            write(getValue() | CCM_CSCMR1_SAI1_CLK_SEL((int) selector));
            return true;
    }
}
