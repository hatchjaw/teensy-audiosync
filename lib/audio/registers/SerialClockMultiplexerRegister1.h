#ifndef SERIALCLOCKMULTIPLEXERREGISTER1_H
#define SERIALCLOCKMULTIPLEXERREGISTER1_H

#include "IMXRT1060Register.h"

/**
 * CCM Serial Clock Multiplexer Register 1 (CCM_CSCMR1)
 * i.MX RT1060 Processor Reference Manual rev. 3, §14.7.7, p. 1051
 * Implementation of audio-related fields.
 * For testability, this class is not implemented as a singleton, but only one
 * instance of it should exist.
 *
 * "NOTE
 * Any change on the above multiplexer will have to be done
 * while the module that its clock is affected is not functional and
 * the clock is gated. If the change will be done during operation
 * of the module, then it is not guaranteed that the modules
 * operation will not be harmed."
 */
class SerialClockMultiplexerRegister1 final : public IMXRT1060Register
{
public:
    enum class Sai1ClkSel : int
    {
        DeriveClockFromPll3Pfd2 = 0,
        DeriveClockFromPll5 = 1,
        DeriveClockFromPll4 = 2,
        Reserved = 3
    };

    SerialClockMultiplexerRegister1()
        : IMXRT1060Register("CCM_CSCMR1", &CCM_CSCMR1) {}

    bool begin() override;

    bool setSai1ClkSel(Sai1ClkSel selector) const;
};


#endif //SERIALCLOCKMULTIPLEXERREGISTER1_H
