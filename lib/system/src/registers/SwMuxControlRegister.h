#ifndef SWMUXCONTROLREGISTER_H
#define SWMUXCONTROLREGISTER_H

#include "IMXRT1060Register.h"

class SwMuxControlRegister : public IMXRT1060Register
{
public:
    enum class SoftwareInputStatus : int
    {
        Disabled = 0,
        Enabled = 1
    };

    bool begin() override;

    void reset() const;

    bool setSoftwareInputOnField(SoftwareInputStatus siOn) const;

protected:
    SwMuxControlRegister(const char *name, volatile uint32_t *address)
        : IMXRT1060Register(name, address)
    {
    }

    bool setMuxMode(const uint32_t mode) const;

private:
    static constexpr int32_t k_Reset{5};
};

//==============================================================================

class Pin7SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        LCD_DATA13 = 0,
        XBAR1_INOUT15 = 1,
        LPUART4_RX = 2,
        SAI1_TX_DATA00 = 3,
        FLEXIO2_FLEXIO17 = 4,
        GPIO2_IO17 = 5,
        FLEXPWM1_PWMB03 = 6,
        ENET2_RDATA00 = 8,
        FLEXIO3_FLEXIO17 = 9
    };

    Pin7SwMuxControlRegister()
        : SwMuxControlRegister("PIN7: IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_01", &CORE_PIN7_CONFIG)
    {
    }

    bool setMuxMode(MuxMode mode) const;
};

//==============================================================================

class Pin20SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        FLEXSPIA_DATA03 = 0,
        WDOG1_B = 1,
        LPUART8_TX = 2,
        SAI1_RX_SYNC = 3,
        CSI_DATA07 = 4,
        GPIO1_IO26 = 5,
        USDHC2_WP = 6,
        KPP_ROW02 = 7,
        ENET2_1588_EVENT1_OUT = 8,
        FLEXIO3_FLEXIO10 = 9
    };

    Pin20SwMuxControlRegister()
        : SwMuxControlRegister("PIN20: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_10", &CORE_PIN20_CONFIG)
    {
    }

    bool setMuxMode(MuxMode mode) const;
};

//==============================================================================

class Pin21SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        FLEXSPIA_DATA02 = 0,
        EWM_OUT_B = 1,
        LPUART8_RX = 2,
        SAI1_RX_BCLK = 3,
        CSI_DATA06 = 4,
        GPIO1_IO27 = 5,
        USDHC2_RESET_B = 6,
        KPP_COL02 = 7,
        ENET2_1588_EVENT1_IN = 8,
        FLEXIO3_FLEXIO11 = 9
    };

    Pin21SwMuxControlRegister()
        : SwMuxControlRegister("PIN21: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_11", &CORE_PIN21_CONFIG)
    {
    }

    bool setMuxMode(MuxMode mode) const;
};

//==============================================================================

class Pin23SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        FLEXSPIA_DQS = 0,
        FLEXPWM4_PWMA01 = 1,
        FLEXCAN1_RX = 2,
        SAI1_MCLK = 3,
        CSI_DATA08 = 4,
        GPIO1_IO25 = 5,
        USDHC2_CLK = 6,
        KPP_COL03 = 7,
        FLEXIO3_FLEXIO09 = 9
    };

    Pin23SwMuxControlRegister()
        : SwMuxControlRegister("PIN23: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_09", &CORE_PIN23_CONFIG)
    {
    }

    bool setMuxMode(MuxMode mode) const;
};

//==============================================================================

class Pin24SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        LPI2C4_SCL = 0,
        CCM_PMIC_READY = 1,
        LPUART1_TX = 2,
        WDOG2_WDOG_B = 3,
        FLEXPWM1_PWMX02 = 4,
        GPIO1_IO12 = 5,
        ENET_1588_EVENT1_OUT = 6,
        NMI_GLUE_NMI = 7
    };

    Pin24SwMuxControlRegister()
        : SwMuxControlRegister("PIN24: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_12", &CORE_PIN24_CONFIG)
    {
    }

    bool setMuxMode(MuxMode mode) const;
};

#endif //SWMUXCONTROLREGISTER_H
