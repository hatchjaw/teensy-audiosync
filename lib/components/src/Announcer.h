#ifndef ANNOUNCER_H
#define ANNOUNCER_H

#include "NetworkProcessor.h"
#include "ProgramComponent.h"

namespace ananas
{
    template<typename PacketType>
    class Announcer final : public ProgramComponent,
                            public NetworkProcessor
    {
    public:
        Announcer(const uint16_t port, const uint transmitInterval)
            : port(port), transmitInterval(transmitInterval)
        {
        }

        void begin() override
        {
            computeSerialNumber();
        }

        void run() override
        {
            if (elapsed > transmitInterval) {
                socket.beginPacket(Constants::MulticastIP, port);
                socket.write(txPacket.rawData(), sizeof(PacketType));
                socket.endPacket();
                elapsed = 0;
            }
        }

        void connect() override
        {
            socket.beginMulticast(Constants::MulticastIP, port);
        }

        PacketType txPacket{};

    private:
        static_assert(sizeof(decltype(PacketType::serial)) > 0,
                      "PacketType must have a `serial` member.");

        void computeSerialNumber()
        {
            uint32_t num{HW_OCOTP_MAC0 & 0xFFFFFF};
            if (num < 10000000) num *= 10;
            txPacket.serial = num;
        }

        uint16_t port;
        uint transmitInterval;
        elapsedMillis elapsed;
    };
} // ananas

#endif //ANNOUNCER_H
