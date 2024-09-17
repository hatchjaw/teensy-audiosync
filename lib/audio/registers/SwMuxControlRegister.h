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
            : IMXRT1060Register(name, address) {}

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
        LcdData13 = 0,
        XBar1InOut15 = 1,
        LpUart4Rx = 2,
        Sai1TxData00 = 3,
        FlexIo2FlexIo17 = 4,
        Gpio2Io17 = 5,
        FlexPwm1PwmB03 = 6,
        Enet2RData00 = 8,
        FlexIo3FlexIo17 = 9
    };

    static Pin7SwMuxControlRegister &instance();

    bool setMuxMode(MuxMode mode) const;

private:
    Pin7SwMuxControlRegister()
            : SwMuxControlRegister("PIN7: IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_01", &CORE_PIN7_CONFIG) {}

    static Pin7SwMuxControlRegister s_Instance;
};

//==============================================================================

class Pin20SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        FlexSpiAData03 = 0,
        Wdog1B = 1,
        LpUart8Tx = 2,
        Sai1RxSync = 3,
        CsiData07 = 4,
        Gpio1Io26 = 5,
        Usdhc2Wp = 6,
        KppRow02 = 7,
        Enet21588Event1Out = 8,
        FlexIo3FlexIo10 = 9
    };

    static Pin20SwMuxControlRegister &instance();

    bool setMuxMode(MuxMode mode) const;

private:
    Pin20SwMuxControlRegister()
            : SwMuxControlRegister("PIN20: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_10", &CORE_PIN20_CONFIG) {}

    static Pin20SwMuxControlRegister s_Instance;
};

//==============================================================================

class Pin21SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        FlexSpiAData02 = 0,
        EwmOutB = 1,
        LpUart8Rx = 2,
        Sai1RxBclk = 3,
        CsiData06 = 4,
        GpioIo27 = 5,
        Usdhc2ResetB = 6,
        KppCol02 = 7,
        Enet21588Event1In = 8,
        FlexIo3FlexIo11 = 9
    };

    static Pin21SwMuxControlRegister &instance();

    bool setMuxMode(MuxMode mode) const;

private:
    Pin21SwMuxControlRegister()
            : SwMuxControlRegister("PIN21: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_11", &CORE_PIN21_CONFIG) {}

    static Pin21SwMuxControlRegister s_Instance;
};

//==============================================================================

class Pin23SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        FlexSpiaDqs = 0,
        FlexPwm4PwmA01 = 1,
        FlexCan1Rx = 2,
        Sai1Mclk = 3,
        CsiData08 = 4,
        GpioIo25 = 5,
        Usdhc2Clk = 6,
        KppCol03 = 7,
        FlexIo3FlexIo09 = 9
    };

    static Pin23SwMuxControlRegister &instance();

    bool setMuxMode(MuxMode mode) const;

private:
    Pin23SwMuxControlRegister()
            : SwMuxControlRegister("PIN23: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_09", &CORE_PIN23_CONFIG) {}

    static Pin23SwMuxControlRegister s_Instance;
};

#endif //SWMUXCONTROLREGISTER_H
