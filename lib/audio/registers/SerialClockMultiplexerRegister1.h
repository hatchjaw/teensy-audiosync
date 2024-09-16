#ifndef SERIALCLOCKMULTIPLEXERREGISTER1_H
#define SERIALCLOCKMULTIPLEXERREGISTER1_H

#include "IMXRT1060Register.h"

/**
 * CCM Serial Clock Multiplexer Register 1 (CCM_CSCMR1)
 * i.MX RT1060 Processor Reference Manual v3, §14.7.7, p. 1051
 * Implementation of audio-related fields
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
        kDeriveClockFromPll3Pfd2 = 0,
        kDeriveClockFromPll5 = 1,
        kDeriveClockFromPll4 = 2,
        kReserved = 3
    };

    static SerialClockMultiplexerRegister1 &instance();

    bool begin() override;

    bool setSai1ClkSel(Sai1ClkSel selector) const;

private:
    SerialClockMultiplexerRegister1()
            : IMXRT1060Register("CCM_CSCMR1", &CCM_CSCMR1) {}

    static SerialClockMultiplexerRegister1 instance_;
};


#endif //SERIALCLOCKMULTIPLEXERREGISTER1_H
