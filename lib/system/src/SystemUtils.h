#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <Arduino.h>

class SystemUtils
{
public:
    static uint32_t computeSerialNumber()
    {
        uint32_t num{HW_OCOTP_MAC0 & 0xFFFFFF};
        if (num < 10000000) num *= 10;
        return num;
    }

    static void reboot()
    {
        SRC_GPR5 = 0x0BAD00F1;
        SCB_AIRCR = 0x05FA0004;
        while (true) {
        }
    }
};

#endif //SYSTEMUTILS_H
