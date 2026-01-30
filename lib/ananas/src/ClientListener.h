#ifndef CLIENTLISTENER_H
#define CLIENTLISTENER_H

#include <map>
#include <AnanasPacket.h>

#include "NetworkProcessor.h"
#include "ProgramComponent.h"

namespace ananas
{
    class ClientListener final : public NetworkProcessor,
                                 public ProgramComponent
    {
    public:
        void connect() override;

    protected:
        void beginImpl() override;

    public:
        void run() override;

        int getNumClients() const;

        int getAvgBufferFill() const;

        size_t printTo(Print &p) const override;

    private:
        struct Client
        {
            ClientAnnouncePacket info;
            elapsedMillis timeSinceLastReceived{0};
        };

        ClientAnnouncePacket rxPacket{};
        std::map<uint32_t, Client> clients;
        elapsedMillis evaluationInterval;
        int avgBufferFillPercent{50};
    };
} // ananas

#endif //CLIENTLISTENER_H
