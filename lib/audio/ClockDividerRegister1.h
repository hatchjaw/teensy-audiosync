#ifndef INC_1588_CLOCKDIVIDERREGISTER1_H
#define INC_1588_CLOCKDIVIDERREGISTER1_H

#include <IMXRT1060Register.h>

/**
 * CCM Clock Divider Register (CCM_CS1CDR)
 * i.MX RT1060 Processor Reference Manual, §14.7.10, p. 1056
 */
class ClockDividerRegister1 final : public IMXRT1060Register
{
public:
    static ClockDividerRegister1 &instance();

    bool begin() override;

    bool setSai1ClkPred(uint8_t divider);

    bool setSai1ClkPodf(uint8_t divider);

private:
    ClockDividerRegister1()
            : IMXRT1060Register("CCM_CS1CDR", &CCM_CS1CDR) {}

    static ClockDividerRegister1 instance_;
};


#endif //INC_1588_CLOCKDIVIDERREGISTER1_H
