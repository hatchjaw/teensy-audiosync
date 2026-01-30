#include <AnanasUtils.h>
#include "ClientListener.h"

namespace ananas
{
    void ClientListener::beginImpl()
    {
    }

    void ClientListener::connect()
    {
        socket.beginMulticast(Constants::ClientAnnounceMulticastIP, Constants::ClientAnnouncePort);
    }

    void ClientListener::run()
    {
        if (const auto size{socket.parsePacket()}; size > 0) {
            socket.read(rxPacket.rawData(), size);

            auto it{clients.find(socket.remoteIP())};
            if (it == clients.end()) {
                it = clients.insert(std::make_pair((uint32_t) socket.remoteIP(), Client())).first;
            }
            it->second.timeSinceLastReceived = 0;
            it->second.info = rxPacket;
        }

        if (evaluationInterval > Constants::GroupEvaluationIntervalMs) {
            evaluationInterval = 0;
            int bufferFillTotal{0};

            // Check client disconnections
            for (auto it{clients.begin()}, next{it}; it != clients.end(); it = next) {
                ++next;
                if (it->second.timeSinceLastReceived > Constants::GroupEvaluationIntervalMs) {
                    clients.erase(it);
                } else {
                    bufferFillTotal += it->second.info.bufferFillPercent;
                }
            }

            avgBufferFillPercent = bufferFillTotal / clients.size();
        }
    }

    int ClientListener::getNumClients() const
    {
        return clients.size();
    }

    int ClientListener::getAvgBufferFill() const
    {
        return avgBufferFillPercent;
    }

    size_t ClientListener::printTo(Print &p) const
    {
        return 0;
    }
} // ananas
