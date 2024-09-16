#ifndef CLOCKDIVIDERREGISTER1_H
#define CLOCKDIVIDERREGISTER1_H

#include "IMXRT1060Register.h"

/**
 * CCM Clock Divider Register (CCM_CS1CDR)
 * i.MX RT1060 Processor Reference Manual v3, §14.7.10, p. 1056
 * Implementation of audio-related fields
 */
class ClockDividerRegister1 final : public IMXRT1060Register
{
public:
    static ClockDividerRegister1 &instance();

    bool begin() override;

    bool setSai1ClkPred(uint8_t divider) const;

    bool setSai1ClkPodf(uint8_t divider) const;

private:
    ClockDividerRegister1()
            : IMXRT1060Register("CCM_CS1CDR", &CCM_CS1CDR) {}

    static ClockDividerRegister1 instance_;
};


#endif //CLOCKDIVIDERREGISTER1_H
