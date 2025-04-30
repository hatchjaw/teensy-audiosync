/*******************************************************************************
 * A collection of classes for managing registers that are relevant to the
 * IMXRT1060's Synchronous Audio Interface (SAI1 --- "1" because there are also
 * SAI2 and SAI3).
 ******************************************************************************/

#ifndef SAI1_H
#define SAI1_H

#include "IMXRT1060Register.h"

class SAI1TransmitControlRegister final : public IMXRT1060Register
{
public:
    SAI1TransmitControlRegister()
        : IMXRT1060Register("SAI1_TCSR", &I2S1_TCSR)
    {
    }

    bool begin() override;

    bool setTransmitterEnable(bool enable) const;

    bool setStopEnable(bool enableInStopMode) const;

    bool setDebugEnable(bool enable) const;

    bool setBitClockEnable(bool enable) const;

    bool resetFIFO() const;

    void setSoftwareReset() const;

    void clearSoftwareReset() const;

    bool clearWordStartFlag() const;

    bool clearSyncErrorFlag() const;

    bool clearFIFOErrorFlag() const;

    bool setWordStartInterruptEnable(bool enable) const;

    bool setSyncErrorInterruptEnable(bool enable) const;

    bool setFIFOErrorInterruptEnable(bool enable) const;

    bool setFIFOWarningInterruptEnable(bool enable) const;

    bool setFIFORequestInterruptEnable(bool enable) const;

    bool setFIFOWarningDMAEnable(bool enable) const;

    bool setFIFORequestDMAEnable(bool enable) const;
};

class SAI1TransmitConfig1Register final : public IMXRT1060Register
{
public:
    SAI1TransmitConfig1Register()
        : IMXRT1060Register("SAI1_TCR1", &I2S1_TCR1)
    {
    }

    bool begin() override;

    bool setWatermarkLevel(uint level) const;
};

class SAI1TransmitConfig2Register final : public IMXRT1060Register
{
public:
    enum class SyncMode : uint
    {
        Asynchronous = 0,
        SynchronousWithReceiver
    };

    enum class BitClockPolarity : uint
    {
        ActiveHigh = 0,
        ActiveLow
    };

    enum class BitClockDirection : uint
    {
        External = 0,
        Internal
    };

    enum class Clock : uint
    {
        BusClock = 0,
        MasterClock1,
        MasterClock2,
        MasterClock3,
    };

    SAI1TransmitConfig2Register()
        : IMXRT1060Register("SAI1_TCR2", &I2S1_TCR2)
    {
    }

    bool begin() override;

    void setSyncMode(SyncMode mode) const;

    void setBitClockPolarity(BitClockPolarity polarity) const;

    void setBitClockDirection(BitClockDirection direction) const;

    bool setBitClockDivide(uint div) const;

    void selectMasterClock(Clock clock) const;
};

class SAI1TransmitConfig3Register final : public IMXRT1060Register
{
public:
    SAI1TransmitConfig3Register()
        : IMXRT1060Register("SAI1_TCR3", &I2S1_TCR3)
    {
    }

    bool begin() override;

    bool enableTransmitChannel(uint channel, bool enable) const;
};

class SAI1TransmitConfig4Register final : public IMXRT1060Register
{
public:
    enum class Endianness : uint
    {
        LittleEndian = 0,
        BigEndian
    };

    enum class FrameSyncDirection : uint
    {
        External = 0,
        Internal
    };

    enum class FrameSyncAssert : uint
    {
        WithFirstBit = 0,
        OneBitEarly
    };

    enum class FrameSyncPolarity : uint
    {
        ActiveHigh = 0,
        ActiveLow
    };

    SAI1TransmitConfig4Register()
        : IMXRT1060Register("SAI1_TCR4", &I2S1_TCR4)
    {
    }

    bool begin() override;

    bool setFrameSize(uint numWords) const;

    bool setSyncWidth(uint numBitClocks) const;

    void setEndianness(Endianness endianness) const;

    void setFrameSyncDirection(FrameSyncDirection direction) const;

    void setFrameSyncEarly(FrameSyncAssert assert) const;

    void setFrameSyncPolarity(FrameSyncPolarity polarity) const;
};

class SAI1TransmitConfig5Register final : public IMXRT1060Register
{
public:
    SAI1TransmitConfig5Register()
        : IMXRT1060Register("SAI1_TCR5", &I2S1_TCR5)
    {
    }

    bool begin() override;

    bool setWordNWidth(uint numBits) const;

    bool setWord0Width(uint numBits) const;

    bool setFirstBitShift(uint bitIndex) const;
};

/**
 * SAI1 Transmit Mask (TMR)
 * i.MX RT1060 Processor Reference Manual rev. 3, §38.5.1.12, p. 2017
 */
class SAI1TransmitMaskRegister final : public IMXRT1060Register
{
public:
    SAI1TransmitMaskRegister()
        : IMXRT1060Register("SAI1_TMR", &I2S1_TMR)
    {
    }

    bool begin() override;

    bool setMask(uint32_t mask) const;
};

class SAI1ReceiveControlRegister final : public IMXRT1060Register
{
public:
    SAI1ReceiveControlRegister()
        : IMXRT1060Register("SAI1_RCSR", &I2S1_RCSR)
    {
    }

    bool begin() override;

    bool setReceiverEnable(bool enable) const;

    bool setStopEnable(bool enableInStopMode) const;

    bool setDebugEnable(bool enable) const;

    bool setBitClockEnable(bool enable) const;

    bool resetFIFO() const;

    bool softwareReset() const;

    bool clearWordStartFlag() const;

    bool clearSyncErrorFlag() const;

    bool clearFIFOErrorFlag() const;

    bool setWordStartInterruptEnable(bool enable) const;

    bool setSyncErrorInterruptEnable(bool enable) const;

    bool setFIFOErrorInterruptEnable(bool enable) const;

    bool setFIFOWarningInterruptEnable(bool enable) const;

    bool setFIFORequestInterruptEnable(bool enable) const;

    bool setFIFOWarningDMAEnable(bool enable) const;

    bool setFIFORequestDMAEnable(bool enable) const;
};

class SAI1ReceiveConfig1Register final : public IMXRT1060Register
{
public:
    SAI1ReceiveConfig1Register()
        : IMXRT1060Register("SAI1_RCR1", &I2S1_RCR1)
    {
    }

    bool begin() override;

    bool setWatermarkLevel(uint level) const;
};

class SAI1ReceiveConfig2Register final : public IMXRT1060Register
{
public:
    enum class SyncMode : uint
    {
        Asynchronous = 0,
        SynchronousWithTransmitter
    };

    enum class BitClockPolarity : uint
    {
        ActiveHigh = 0,
        ActiveLow
    };

    enum class BitClockDirection : uint
    {
        External = 0,
        Internal
    };

    enum class Clock : uint
    {
        BusClock = 0,
        MasterClock1,
        MasterClock2,
        MasterClock3,
    };

    SAI1ReceiveConfig2Register()
        : IMXRT1060Register("SAI1_RCR2", &I2S1_RCR2)
    {
    }

    bool begin() override;

    void setSyncMode(SyncMode mode) const;

    void setBitClockPolarity(BitClockPolarity polarity) const;

    void setBitClockDirection(BitClockDirection direction) const;

    bool setBitClockDivide(uint div) const;

    void selectMasterClock(Clock clock) const;
};

class SAI1ReceiveConfig3Register final : public IMXRT1060Register
{
public:
    SAI1ReceiveConfig3Register()
        : IMXRT1060Register("SAI1_RCR3", &I2S1_RCR3)
    {
    }

    bool begin() override;

    bool enableReceiveChannel(uint channel, bool enable) const;
};

class SAI1ReceiveConfig4Register final : public IMXRT1060Register
{
public:
    enum class Endianness : uint
    {
        LittleEndian = 0,
        BigEndian
    };

    enum class FrameSyncDirection : uint
    {
        External = 0,
        Internal
    };

    enum class FrameSyncAssert : uint
    {
        WithFirstBit = 0,
        OneBitEarly
    };

    enum class FrameSyncPolarity : uint
    {
        ActiveHigh = 0,
        ActiveLow
    };

    SAI1ReceiveConfig4Register()
        : IMXRT1060Register("SAI1_RCR4", &I2S1_RCR4)
    {
    }

    bool begin() override;

    bool setFrameSize(uint numWords) const;

    bool setSyncWidth(uint numBitClocks) const;

    void setEndianness(Endianness endianness) const;

    void setFrameSyncDirection(FrameSyncDirection direction) const;

    void setFrameSyncEarly(FrameSyncAssert assert) const;

    void setFrameSyncPolarity(FrameSyncPolarity polarity) const;
};

class SAI1ReceiveConfig5Register final : public IMXRT1060Register
{
public:
    SAI1ReceiveConfig5Register()
        : IMXRT1060Register("SAI1_RCR5", &I2S1_RCR5)
    {
    }

    bool begin() override;

    bool setWordNWidth(uint numBits) const;

    bool setWord0Width(uint numBits) const;

    bool setFirstBitShift(uint bitIndex) const;
};

class SAI1ReceiveMaskRegister final : public IMXRT1060Register
{
public:
    SAI1ReceiveMaskRegister()
        : IMXRT1060Register("SAI1_RMR", &I2S1_RMR)
    {
    }

    bool begin() override;

    bool setMask(uint32_t mask) const;
};

#endif //SAI1_H
