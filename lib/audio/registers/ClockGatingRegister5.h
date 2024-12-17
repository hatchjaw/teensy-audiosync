#ifndef CLOCKGATINGREGISTER5_H
#define CLOCKGATINGREGISTER5_H

#include "IMXRT1060Register.h"

/**
 * CCM Clock Gating Register 5 (CCM_CCGR5)
 * i.MX RT1060 Processor Reference Manual rev. 3, §14.7.26, p. 1084
 * Implementation of audio-related fields.
 * Only one instance of this class should exist.
 */
class ClockGatingRegister5 final : public IMXRT1060Register
{
public:
    /**
     * §14.7.21, p. 1077
     * "These bits are used to turn on/off the clock to each module independently.
     * The following table details the possible clock activity conditions for
     * each module.
     * Module should be stopped, before set its bits to "0"; clocks to the
     * module will be stopped immediately."
     */
    enum class ClockActivityCondition : int
    {
        /**
         * Clock is off during all modes. Stop enter hardware handshake is disabled.
         */
        Off = 0,
        /**
         * Clock is on in run mode, but off in WAIT and STOP modes.
         */
        OnRunOnly = 1,
        /**
         * Clock is on during all modes, except STOP mode.
         */
        On = 3
    };

    ClockGatingRegister5() : IMXRT1060Register("CCM_CCGR5", &CCM_CCGR5)
    {
    }

    bool begin() override;

    bool enableSai1Clock() const;

    bool setSai1ClockActivityCondition(ClockActivityCondition condition) const;
};


#endif //CLOCKGATINGREGISTER5_H
