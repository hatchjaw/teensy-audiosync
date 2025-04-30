#include "ClockGatingRegister5.h"

bool ClockGatingRegister5::begin()
{
    return true;
}

bool ClockGatingRegister5::enableSai1Clock() const
{
    return setSai1ClockActivityCondition(ClockActivityCondition::On);
}

bool ClockGatingRegister5::setSai1ClockActivityCondition(const ClockActivityCondition condition) const
{
    write(getValue() | CCM_CCGR5_SAI1((int) condition));
    return true;
}
