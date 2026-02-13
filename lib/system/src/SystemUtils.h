#ifndef SYSTEMUTILS_H
#define SYSTEMUTILS_H

#include <Arduino.h>

class SystemUtils
{
public:
    // 0 = highest priority
    // Cortex-M7: 0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,240
    enum IrqPriority : uint8_t
    {
        Priority0 = 0,
        Priority16 = 16,
        Priority32 = 32,
        Priority48 = 48,
        Priority64 = 64,
        Priority80 = 80,
        Priority96 = 96,
        Priority112 = 112,
        Priority128 = 128,
        Priority144 = 144,
        Priority160 = 160,
        Priority176 = 176,
        Priority192 = 192,
        Priority208 = 208,
        Priority224 = 224,
        Priority240 = 240,
    };

    enum LogLevel {
        None = 0,
        Low = 1,
        Medium,
        High
    };

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
