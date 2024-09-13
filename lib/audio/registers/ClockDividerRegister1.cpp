#include "ClockDividerRegister1.h"
#include "ClockConstants.h"

ClockDividerRegister1 ClockDividerRegister1::instance_;

ClockDividerRegister1 &ClockDividerRegister1::instance()
{
    return instance_;
}

ClockDividerRegister1 &ClockDividerRegister1 = ClockDividerRegister1::instance();

bool ClockDividerRegister1::begin()
{
    // Clear SAI1 dividers.
    write(CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK));
    return true;
}

bool ClockDividerRegister1::setSai1ClkPred(uint8_t divider) const
{
    if (divider == 0 || divider > ClockConstants::k_sai1PreMax) {
        Serial.printf("Invalid value provided for SAI1_CLK_PRED: "
                      "%" PRIu8 "\n",
                      divider);
        return false;
    }

    write(getValue() | CCM_CS1CDR_SAI1_CLK_PRED(divider - 1));
    return true;
}

bool ClockDividerRegister1::setSai1ClkPodf(uint8_t divider) const
{
    if (divider == 0 || divider > ClockConstants::k_sai1PostMax) {
        Serial.printf("Invalid value provided for SAI1_CLK_PODF: "
                      "%" PRIu8 "\n",
                      divider);
        return false;
    }

    write(getValue() | CCM_CS1CDR_SAI1_CLK_PODF(divider - 1));
    return true;
}
