#include "SAI1.h"

bool SAI1TransmitControlRegister::begin()
{
    write(0);
    return true;
}

bool SAI1TransmitControlRegister::setTransmitterEnable(const bool enable) const
{
    writeMask(enable, I2S_TCSR_TE);
    if (!enable) {
        while (getValue() & I2S_TCSR_TE) {}
    }
    return true;
}

bool SAI1TransmitControlRegister::setStopEnable(const bool enableInStopMode) const
{
    writeMask(enableInStopMode, 0x40000000);
    return true;
}

bool SAI1TransmitControlRegister::setDebugEnable(const bool enable) const
{
    writeMask(enable, 0x20000000);
    return true;
}

bool SAI1TransmitControlRegister::setBitClockEnable(const bool enable) const
{
    writeMask(enable, I2S_TCSR_BCE);
    return true;
}

bool SAI1TransmitControlRegister::resetFIFO() const
{
    write(getValue() | 0x40000 | I2S_TCSR_FR);
    return true;
}

void SAI1TransmitControlRegister::setSoftwareReset() const
{
    write(getValue() | 0x1000000);
}

void SAI1TransmitControlRegister::clearSoftwareReset() const
{
    write(getValue() & ~0x1000000);
}

bool SAI1TransmitControlRegister::clearWordStartFlag() const
{
    write(getValue() | 0x100000);
    return true;
}

bool SAI1TransmitControlRegister::clearSyncErrorFlag() const
{
    write(getValue() | 0x80000);
    return true;
}

bool SAI1TransmitControlRegister::clearFIFOErrorFlag() const
{
    write(getValue() | 0x40000);
    return true;
}

bool SAI1TransmitControlRegister::setWordStartInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x1000);
    return true;
}

bool SAI1TransmitControlRegister::setSyncErrorInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x800);
    return true;
}

bool SAI1TransmitControlRegister::setFIFOErrorInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x400);
    return true;
}

bool SAI1TransmitControlRegister::setFIFOWarningInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x200);
    return true;
}

bool SAI1TransmitControlRegister::setFIFORequestInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x100);
    return true;
}

bool SAI1TransmitControlRegister::setFIFOWarningDMAEnable(bool const enable) const
{
    writeMask(enable, 2);
    return true;
}

bool SAI1TransmitControlRegister::setFIFORequestDMAEnable(bool const enable) const
{
    writeMask(enable, I2S_TCSR_FRDE);
    return true;
}

bool SAI1TransmitConfig1Register::begin()
{
    write(0);
    return true;
}

bool SAI1TransmitConfig1Register::setWatermarkLevel(uint const level) const
{
    if (level > 0x1F) {
        Serial.printf("Invalid SAI1 watermark level: %d > 31\n", level);
        return false;
    }

    write(level);
    return true;
}

bool SAI1TransmitConfig2Register::begin()
{
    write(0);
    return true;
}

void SAI1TransmitConfig2Register::setSyncMode(SyncMode mode) const
{
    write(getValue() | I2S_TCR2_SYNC((uint32_t)mode));
}

void SAI1TransmitConfig2Register::setBitClockPolarity(const BitClockPolarity polarity) const
{
    writeMask(polarity == BitClockPolarity::ActiveLow, I2S_TCR2_BCP);
}

void SAI1TransmitConfig2Register::setBitClockDirection(const BitClockDirection direction) const
{
    writeMask(direction == BitClockDirection::Internal, I2S_TCR2_BCD);
}

bool SAI1TransmitConfig2Register::setBitClockDivide(const uint div) const
{
    if (div > 0xFF) {
        Serial.printf("Invalid SAI1 bit clock divider value: %d > 255\n", div);
        return false;
    }

    write(getValue() | I2S_TCR2_DIV(div));
    return true;
}

void SAI1TransmitConfig2Register::selectMasterClock(const Clock clock) const
{
    write(getValue() | I2S_TCR2_MSEL((uint32_t)clock));
}

bool SAI1TransmitConfig3Register::begin()
{
    write(0);
    return true;
}

bool SAI1TransmitConfig3Register::enableTransmitChannel(const uint channel, const bool enable) const
{
    if (channel < 1 || channel > 4) {
        Serial.printf("Invalid SAI1 transmit channel: %d\n", channel);
        return false;
    }

    const auto mask{(1 << (channel - 1)) << 16};

    writeMask(enable, mask);
    return true;
}

bool SAI1TransmitConfig4Register::begin()
{
    write(0);
    return true;
}

bool SAI1TransmitConfig4Register::setFrameSize(const uint numWords) const
{
    if (numWords > 0x1F) {
        Serial.printf("Invalid SAI1 transmit frame size: %d > 31\n", numWords);
        return false;
    }

    write(getValue() | I2S_TCR4_FRSZ(numWords - 1));
    return true;
}

bool SAI1TransmitConfig4Register::setSyncWidth(const uint numBitClocks) const
{
    // TODO: "The sync width cannot be configured longer than the first word of the frame."

    write(getValue() | I2S_TCR4_SYWD(numBitClocks - 1));
    return true;
}

void SAI1TransmitConfig4Register::setEndianness(const Endianness endianness) const
{
    writeMask(endianness == Endianness::BigEndian, I2S_TCR4_MF);
}

void SAI1TransmitConfig4Register::setFrameSyncDirection(const FrameSyncDirection direction) const
{
    writeMask(direction == FrameSyncDirection::Internal, I2S_TCR4_FSD);
}

void SAI1TransmitConfig4Register::setFrameSyncEarly(const FrameSyncAssert assert) const
{
    writeMask(assert == FrameSyncAssert::OneBitEarly, I2S_TCR4_FSE);
}

void SAI1TransmitConfig4Register::setFrameSyncPolarity(const FrameSyncPolarity polarity) const
{
    writeMask(polarity == FrameSyncPolarity::ActiveLow, I2S_TCR4_FSP);
}

bool SAI1TransmitConfig5Register::begin()
{
    write(0);
    return true;
}

bool SAI1TransmitConfig5Register::setWordNWidth(const uint numBits) const
{
    if (numBits > 32 || numBits < 8) {
        Serial.printf("Invalid SAI1 transmit word width: %d\n", numBits);
        return false;
    }

    write(getValue() | I2S_TCR5_WNW(numBits - 1));
    return true;
}

bool SAI1TransmitConfig5Register::setWord0Width(const uint numBits) const
{
    // TODO: "Word width of less than 8 bits is not supported if there is only one word per frame."

    write(getValue() | I2S_TCR5_W0W(numBits - 1));
    return true;
}

bool SAI1TransmitConfig5Register::setFirstBitShift(const uint bitIndex) const
{
    write(getValue() | I2S_TCR5_FBT(bitIndex - 1));
    return true;
}

bool SAI1TransmitMaskRegister::begin()
{
    return setMask(0);
}

bool SAI1TransmitMaskRegister::setMask(const uint32_t mask) const
{
    write(mask);
    return true;
}

bool SAI1ReceiveControlRegister::begin()
{
    write(0);
    return true;
}

bool SAI1ReceiveControlRegister::setReceiverEnable(bool enable) const
{
    writeMask(enable, I2S_RCSR_RE);
    if (!enable) {
        while (getValue() & I2S_RCSR_RE) {}
    }
    return true;
}

bool SAI1ReceiveControlRegister::setStopEnable(bool enableInStopMode) const
{
    writeMask(enableInStopMode, 0x40000000);
    return true;
}

bool SAI1ReceiveControlRegister::setDebugEnable(bool enable) const
{
    writeMask(enable, 0x20000000);
    return true;
}

bool SAI1ReceiveControlRegister::setBitClockEnable(bool enable) const
{
    writeMask(enable, I2S_RCSR_BCE);
    return true;
}

bool SAI1ReceiveControlRegister::resetFIFO() const
{
    write(getValue() | I2S_RCSR_FR);
    return true;
}

bool SAI1ReceiveControlRegister::softwareReset() const
{
    write(getValue() | 0x1000000);
    return true;
}

bool SAI1ReceiveControlRegister::clearWordStartFlag() const
{
    write(getValue() | 0x100000);
    return true;
}

bool SAI1ReceiveControlRegister::clearSyncErrorFlag() const
{
    write(getValue() | 0x80000);
    return true;
}

bool SAI1ReceiveControlRegister::clearFIFOErrorFlag() const
{
    write(getValue() | 0x40000);
    return true;
}

bool SAI1ReceiveControlRegister::setWordStartInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x1000);
    return true;
}

bool SAI1ReceiveControlRegister::setSyncErrorInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x800);
    return true;
}

bool SAI1ReceiveControlRegister::setFIFOErrorInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x400);
    return true;
}

bool SAI1ReceiveControlRegister::setFIFOWarningInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x200);
    return true;
}

bool SAI1ReceiveControlRegister::setFIFORequestInterruptEnable(bool const enable) const
{
    writeMask(enable, 0x100);
    return true;
}

bool SAI1ReceiveControlRegister::setFIFOWarningDMAEnable(bool const enable) const
{
    writeMask(enable, 2);
    return true;
}

bool SAI1ReceiveControlRegister::setFIFORequestDMAEnable(bool const enable) const
{
    writeMask(enable, 1);
    return true;
}

bool SAI1ReceiveConfig1Register::begin()
{
    write(0);
    return true;
}

bool SAI1ReceiveConfig1Register::setWatermarkLevel(uint level) const
{
    if (level > 0x1F) {
        Serial.printf("Invalid SAI1 watermark level: %d > 31\n", level);
        return false;
    }

    write(level);
    return true;
}

bool SAI1ReceiveConfig2Register::begin()
{
    write(0);
    return true;
}

void SAI1ReceiveConfig2Register::setSyncMode(SyncMode mode) const
{
    write(getValue() | I2S_RCR2_SYNC((uint32_t)mode));
}

void SAI1ReceiveConfig2Register::setBitClockPolarity(BitClockPolarity polarity) const
{
    writeMask(polarity == BitClockPolarity::ActiveLow, I2S_RCR2_BCP);
}

void SAI1ReceiveConfig2Register::setBitClockDirection(BitClockDirection direction) const
{
    writeMask(direction == BitClockDirection::Internal, I2S_RCR2_BCD);
}

bool SAI1ReceiveConfig2Register::setBitClockDivide(uint div) const
{
    if (div > 0xFF) {
        Serial.printf("Invalid SAI1 bit clock divider value: %d > 255\n", div);
        return false;
    }

    write(getValue() | I2S_RCR2_DIV(div));
    return true;
}

void SAI1ReceiveConfig2Register::selectMasterClock(Clock clock) const
{
    write(getValue() | I2S_TCR2_MSEL((uint32_t)clock));
}

bool SAI1ReceiveConfig3Register::begin()
{
    write(0);
    return true;
}

bool SAI1ReceiveConfig3Register::enableReceiveChannel(uint channel, bool enable) const
{
    if (channel < 1 || channel > 4) {
        Serial.printf("Invalid SAI1 receive channel: %d\n", channel);
        return false;
    }

    const auto mask{(1 << (channel - 1)) << 16};

    writeMask(enable, mask);
    return true;
}

bool SAI1ReceiveConfig4Register::begin()
{
    write(0);
    return true;
}

bool SAI1ReceiveConfig4Register::setFrameSize(uint numWords) const
{
    if (numWords > 0x1F) {
        Serial.printf("Invalid SAI1 receive frame size: %d > 31\n", numWords);
        return false;
    }

    write(getValue() | I2S_RCR4_FRSZ(numWords - 1));
    return true;
}

bool SAI1ReceiveConfig4Register::setSyncWidth(uint numBitClocks) const
{
    // TODO: "The sync width cannot be configured longer than the first word of the frame."

    write(getValue() | I2S_RCR4_SYWD(numBitClocks - 1));
    return true;
}

void SAI1ReceiveConfig4Register::setEndianness(Endianness endianness) const
{
    writeMask(endianness == Endianness::BigEndian, I2S_RCR4_MF);
}

void SAI1ReceiveConfig4Register::setFrameSyncDirection(FrameSyncDirection direction) const
{
    writeMask(direction == FrameSyncDirection::Internal, I2S_RCR4_FSD);
}

void SAI1ReceiveConfig4Register::setFrameSyncEarly(FrameSyncAssert assert) const
{
    writeMask(assert == FrameSyncAssert::OneBitEarly, I2S_RCR4_FSE);
}

void SAI1ReceiveConfig4Register::setFrameSyncPolarity(FrameSyncPolarity polarity) const
{
    writeMask(polarity == FrameSyncPolarity::ActiveLow, I2S_RCR4_FSP);
}

bool SAI1ReceiveConfig5Register::begin()
{
    write(0);
    return true;
}

bool SAI1ReceiveConfig5Register::setWordNWidth(uint numBits) const
{
    if (numBits > 32 || numBits < 8) {
        Serial.printf("Invalid SAI1 receive word width: %d\n", numBits);
        return false;
    }

    write(getValue() | I2S_TCR5_WNW(numBits - 1));
    return true;
}

bool SAI1ReceiveConfig5Register::setWord0Width(uint numBits) const
{
    // TODO: "Word width of less than 8 bits is not supported if there is only one word per frame."

    write(getValue() | I2S_RCR5_W0W(numBits - 1));
    return true;
}

bool SAI1ReceiveConfig5Register::setFirstBitShift(uint bitIndex) const
{
    write(getValue() | I2S_RCR5_FBT(bitIndex - 1));
    return true;
}

bool SAI1ReceiveMaskRegister::begin()
{
    write(0);
    return true;
}

bool SAI1ReceiveMaskRegister::setMask(uint32_t mask) const
{
    write(mask);
    return true;
}
