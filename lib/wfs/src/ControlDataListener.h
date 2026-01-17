#ifndef CONTROLDATALISTENER_H
#define CONTROLDATALISTENER_H

#include "ControlContext.h"
#include <OSCBundle.h>
#include <NetworkProcessor.h>
#include <ProgramComponent.h>

namespace ananas::WFS
{
    class ControlDataListener final : public NetworkProcessor,
                                      public ProgramComponent
    {
    public:
        explicit ControlDataListener(ControlContext &controlContext);

        void connect() override;

        void begin() override;

        void run() override;

    private:
        void parseModule(OSCMessage &msg, int addrOffset) const;

        void parseSpacing(OSCMessage &msg, int addrOffset) const;

        void parsePosition(OSCMessage &msg, int addrOffset) const;

        /**
        * Max message size is ~44 bytes; leave some headroom.
        */
        uint8_t rxBuffer[64]{};
        OSCBundle bundleIn;
        OSCMessage messageIn;
        ControlContext &context;
    };
} // ananas::WFS

#endif //CONTROLDATALISTENER_H
