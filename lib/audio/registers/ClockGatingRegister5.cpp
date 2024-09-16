#include "ClockGatingRegister5.h"

ClockGatingRegister5 ClockGatingRegister5::instance_;

ClockGatingRegister5 &ClockGatingRegister5::instance()
{
    return instance_;
}

ClockGatingRegister5 &ClockGatingRegister5 = ClockGatingRegister5::instance();

bool ClockGatingRegister5::begin()
{
    return true;
}

bool ClockGatingRegister5::enableSai1Clock()
{
    write(getValue() | setSai1ClockActivityCondition(ClockActivityCondition::kOn));
    return true;
}

bool ClockGatingRegister5::setSai1ClockActivityCondition(ClockActivityCondition condition)
{
    write(getValue() | CCM_CCGR5_SAI1((int) condition));
    return true;
}
