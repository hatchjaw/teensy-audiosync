#ifndef SWMUXCONTROLREGISTER_H
#define SWMUXCONTROLREGISTER_H

#include "IMXRT1060Register.h"

class SwMuxControlRegister : public IMXRT1060Register
{
public:
    enum class SoftwareInput : int
    {
        kDisabled = 0,
        kEnabled = 1
    };

    bool begin() override;

    void reset();

    bool setSoftwareInputOnField(SoftwareInput sion) const;

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
        kLcdData13 = 0,
        kXBar1InOut15 = 1,
        kLpUart4Rx = 2,
        kSai1TxData00 = 3,
        kFlexIo2FlexIo17 = 4,
        kGpio2Io17 = 5,
        kFlexPwm1PwmB03 = 6,
        kEnet2RData00 = 8,
        kFlexIo3FlexIo17 = 9
    };

    static Pin7SwMuxControlRegister &instance();

    bool setMuxMode(MuxMode mode) const;

private:
    Pin7SwMuxControlRegister()
            : SwMuxControlRegister("PIN7: IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_01", &CORE_PIN7_CONFIG) {}

    static Pin7SwMuxControlRegister instance_;
};

//==============================================================================

class Pin20SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        kFlexSpiAData03 = 0,
        kWdog1B = 1,
        kLpUart8Tx = 2,
        kSai1RxSync = 3,
        kCsiData07 = 4,
        kGpio1Io26 = 5,
        kUsdhc2Wp = 6,
        kKppRow02 = 7,
        kEnet21588Event1Out = 8,
        kFlexIo3FlexIo10 = 9
    };

    static Pin20SwMuxControlRegister &instance();

    bool setMuxMode(MuxMode mode) const;

private:
    Pin20SwMuxControlRegister()
            : SwMuxControlRegister("PIN20: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_10", &CORE_PIN20_CONFIG) {}

    static Pin20SwMuxControlRegister instance_;
};

//==============================================================================

class Pin21SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        kFlexSpiAData02 = 0,
        kEwmOutB = 1,
        kLpUart8Rx = 2,
        kSai1RxBclk = 3,
        kCsiData06 = 4,
        kGpioIo27 = 5,
        kUsdhc2ResetB = 6,
        kKppCol02 = 7,
        kEnet21588Event1In = 8,
        kFlexIo3FlexIo11 = 9
    };

    static Pin21SwMuxControlRegister &instance();

    bool setMuxMode(MuxMode mode) const;

private:
    Pin21SwMuxControlRegister()
            : SwMuxControlRegister("PIN21: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_11", &CORE_PIN21_CONFIG) {}

    static Pin21SwMuxControlRegister instance_;
};

//==============================================================================

class Pin23SwMuxControlRegister final : public SwMuxControlRegister
{
public:
    enum class MuxMode : uint32_t
    {
        kFlexSpiaDqs = 0,
        kFlexPwm4PwmA01 = 1,
        kFlexCan1Rx = 2,
        kSai1Mclk = 3,
        kCsiData08 = 4,
        kGpioIo25 = 5,
        kUsdhc2Clk = 6,
        kKppCol03 = 7,
        kFlexIo3FlexIo09 = 9
    };

    static Pin23SwMuxControlRegister &instance();

    bool setMuxMode(MuxMode mode) const;

private:
    Pin23SwMuxControlRegister()
            : SwMuxControlRegister("PIN23: IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_09", &CORE_PIN23_CONFIG) {}

    static Pin23SwMuxControlRegister instance_;
};

#endif //SWMUXCONTROLREGISTER_H
