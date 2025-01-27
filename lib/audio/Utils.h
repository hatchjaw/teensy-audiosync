#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <TimeLib.h>

namespace ananas
{
    class Utils
    {
    public:
        static void hexDump(const uint8_t *buffer, const size_t length)
        {
            size_t word{0}, row{0};
            for (const uint8_t *p = buffer; word < length; ++p, ++word) {
                if (word % 16 == 0) {
                    if (word != 0) Serial.print(F("\n"));
                    Serial.printf(F("%04x: "), row);
                    ++row;
                } else if (word % 2 == 0) {
                    Serial.print(F(" "));
                }
                Serial.printf(F("%02x "), *p);
            }
            Serial.println(F("\n"));
        }

        static void printTime(const NanoTime t)
        {
            NanoTime x = t;
            const int ns = x % 1000;
            x /= 1000;
            const int us = x % 1000;
            x /= 1000;
            const int ms = x % 1000;
            x /= 1000;

            tmElements_t tme;
            breakTime(x, tme);

            Serial.printf("%02d.%02d.%04d %02d:%02d:%02d::%03d:%03d:%03d", tme.Day, tme.Month, 1970 + tme.Year, tme.Hour, tme.Minute, tme.Second, ms, us, ns);
        }
    };
}

#endif //UTILS_H
