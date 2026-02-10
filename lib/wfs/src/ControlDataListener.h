#ifndef CONTROLDATALISTENER_H
#define CONTROLDATALISTENER_H

#include <AnanasClient.h>

#include "ControlContext.h"
#include <OSCBundle.h>
#include <ProgramComponent.h>

namespace ananas::WFS
{
    class ControlDataListener final : public ListenerSocket,
                                      public ProgramComponent
    {
    public:
        explicit ControlDataListener(ControlContext &controlContext);

    protected:
        void beginImpl() override;

    public:
        void run() override;

        size_t printTo(Print &p) const override;

    private:
        void parseModule(OSCMessage &msg, int addrOffset) const;

        void parseSpacing(OSCMessage &msg, int addrOffset) const;

        void parsePosition(OSCMessage &msg, int addrOffset) const;

    private:
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
