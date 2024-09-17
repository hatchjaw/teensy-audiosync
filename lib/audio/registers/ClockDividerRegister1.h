#ifndef CLOCKDIVIDERREGISTER1_H
#define CLOCKDIVIDERREGISTER1_H

#include "IMXRT1060Register.h"

/**
 * CCM Clock Divider Register (CCM_CS1CDR)
 * i.MX RT1060 Processor Reference Manual rev. 3, §14.7.10, p. 1056
 * Implementation of audio-related fields.
 * For testability, this class is not implemented as a singleton, but only one
 * instance of it should exist.
 */
class ClockDividerRegister1 final : public IMXRT1060Register
{
public:
    ClockDividerRegister1()
        : IMXRT1060Register("CCM_CS1CDR", &CCM_CS1CDR) {}

    bool begin() override;

    bool setSai1ClkPred(uint8_t divider) const;

    bool setSai1ClkPodf(uint8_t divider) const;
};


#endif //CLOCKDIVIDERREGISTER1_H
