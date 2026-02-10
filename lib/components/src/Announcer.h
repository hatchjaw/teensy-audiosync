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
    protected:
        void beginImpl() override
        {
        }

    public:
        explicit Announcer(const AnnounceSocketParams &p)
            : port(p.port), transmitInterval(p.intervalMs), ip(p.ip)
        {
        }

        void run() override
        {
            if (elapsed > transmitInterval) {
                socket.beginPacket(ip, port);
                socket.write(txPacket.rawData(), sizeof(PacketType));
                socket.endPacket();
                elapsed = 0;
            }
        }

        void connect() override
        {
            socket.beginMulticast(ip, port);
        }

        size_t printTo(Print &p) const override
        {
            return 0;
        }

        PacketType txPacket{};

    private:
        static_assert(sizeof(decltype(PacketType::serial)) > 0,
                      "PacketType must have a `serial` member.");

        uint16_t port;
        uint transmitInterval;
        elapsedMillis elapsed;
        IPAddress ip;
    };
} // ananas

#endif //ANNOUNCER_H
