#include "SwMuxControlRegister.h"

bool SwMuxControlRegister::begin()
{
    reset();
    return true;
}

void SwMuxControlRegister::reset() const
{
    write(k_Reset);
}

bool SwMuxControlRegister::setSoftwareInputOnField(const SoftwareInputStatus siOn) const
{
    writeMask(siOn == SoftwareInputStatus::Enabled, (int) siOn << 0x04);
    // switch (siOn) {
    //     case SoftwareInputStatus::Disabled:
    //         write(getValue() & ~((int) siOn << 0x04));
    //         break;
    //     case SoftwareInputStatus::Enabled:
    //         write(getValue() | ((int) siOn << 0x04));
    //         break;
    // }
    return true;
}

bool SwMuxControlRegister::setMuxMode(const uint32_t mode) const
{
    if (mode > 15) {
        Serial.printf("Invalid value provided for pin mux mode: %" PRIu32 "\n", mode);
        return false;
    }
    write((getValue() & ~0x0F) | mode);
    return true;
}

//==============================================================================

bool Pin7SwMuxControlRegister::setMuxMode(const MuxMode mode) const
{
    return SwMuxControlRegister::setMuxMode((uint32_t) mode);
}

//==============================================================================

bool Pin20SwMuxControlRegister::setMuxMode(const MuxMode mode) const
{
    return SwMuxControlRegister::setMuxMode((uint32_t) mode);
}

//==============================================================================

bool Pin21SwMuxControlRegister::setMuxMode(const MuxMode mode) const
{
    return SwMuxControlRegister::setMuxMode((uint32_t) mode);
}

//==============================================================================

bool Pin23SwMuxControlRegister::setMuxMode(const MuxMode mode) const
{
    return SwMuxControlRegister::setMuxMode((uint32_t) mode);
}
