#include "ControlDataListener.h"

#include <AnanasUtils.h>
#include <QNEthernet.h>

namespace ananas::WFS
{
    ControlDataListener::ControlDataListener(ControlContext &controlContext)
        : context(controlContext)
    {
    }

    void ControlDataListener::beginImpl()
    {
    }

    void ControlDataListener::connect()
    {
        socket.beginMulticast(Constants::WFSControlMulticastIP, Constants::WFSControlPort);
    }

    void ControlDataListener::run()
    {
        if (const auto size{socket.parsePacket()}; size > 0) {
            socket.read(rxBuffer, size);

            // Try to read as bundle
            bundleIn.empty();
            bundleIn.fill(rxBuffer, size);
            if (!bundleIn.hasError() && bundleIn.size() > 0) {
                bundleIn.route("/source", [this](OSCMessage &msg, int addrOffset) { parsePosition(msg, addrOffset); });
                bundleIn.route("/module", [this](OSCMessage &msg, int addrOffset) { parseModule(msg, addrOffset); });
                bundleIn.route("/spacing", [this](OSCMessage &msg, int addrOffset) { parseSpacing(msg, addrOffset); });
            } else {
                // Try as message
                messageIn.empty();
                messageIn.fill(rxBuffer, size);
                if (!messageIn.hasError() && messageIn.size() > 0) {
                    messageIn.route("/source", [this](OSCMessage &msg, int addrOffset) { parsePosition(msg, addrOffset); });
                    messageIn.route("/module", [this](OSCMessage &msg, int addrOffset) { parseModule(msg, addrOffset); });
                    messageIn.route("/spacing", [this](OSCMessage &msg, int addrOffset) { parseSpacing(msg, addrOffset); });
                }
            }
        }
    }

    size_t ControlDataListener::printTo(Print &p) const
    {
        return 0;
    }

    void ControlDataListener::parseModule(OSCMessage &msg, int addrOffset) const
    {
        char ipString[16];
        IPAddress ip;
        msg.getString(0, ipString, 16);
        ip.fromString(ipString);
        if (ip == qindesign::network::Ethernet.localIP()) {
            char id[2];
            msg.getAddress(id, addrOffset + 1);
            const auto numericID{strtof(id, nullptr)};
            // Serial.printf("Receiving module ID: %f\n", numericID);
            context.moduleID = numericID;
        }
    }

    void ControlDataListener::parseSpacing(OSCMessage &msg, int addrOffset) const
    {
        const auto spacing{msg.getFloat(0)};
        // Serial.printf("Receiving \"spacing\": %f\n", spacing);
        context.speakerSpacing = spacing;
    }

    void ControlDataListener::parsePosition(OSCMessage &msg, int addrOffset) const
    {
        // Get the source index and coordinate axis, e.g. "0/x"
        char path[20];
        msg.getAddress(path, addrOffset + 1);
        // Get the coordinate value (-1,1).
        const auto pos{msg.getFloat(0)};
        // Serial.printf("Receiving \"%s\": %f\n", path, pos);
        // Set the parameter.
        auto it{context.sourcePositions.find(path)};
        if (it != context.sourcePositions.end()) {
            it->second.set(pos);
        }
    }
}
