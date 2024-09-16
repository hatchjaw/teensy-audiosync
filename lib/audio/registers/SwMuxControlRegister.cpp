#include "SwMuxControlRegister.h"

bool SwMuxControlRegister::begin()
{
    reset();
    return true;
}

void SwMuxControlRegister::reset()
{
    write(k_Reset);
}

bool SwMuxControlRegister::setSoftwareInputOnField(const SwMuxControlRegister::SoftwareInput sion) const
{
    switch (sion) {
        case SoftwareInput::kDisabled:
            write(getValue() & ~((int) sion << 0x04));
            break;
        case SoftwareInput::kEnabled:
            write(getValue() | ((int) sion << 0x04));
            break;
    }
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

Pin7SwMuxControlRegister Pin7SwMuxControlRegister::instance_;

Pin7SwMuxControlRegister &Pin7SwMuxControlRegister::instance()
{
    return instance_;
}

Pin7SwMuxControlRegister &Pin7SwMuxControlRegister = Pin7SwMuxControlRegister::instance();

bool Pin7SwMuxControlRegister::setMuxMode(const MuxMode mode) const
{
    return SwMuxControlRegister::setMuxMode((uint32_t) mode);
}

//==============================================================================

Pin20SwMuxControlRegister Pin20SwMuxControlRegister::instance_;

Pin20SwMuxControlRegister &Pin20SwMuxControlRegister::instance()
{
    return instance_;
}

Pin20SwMuxControlRegister &Pin20SwMuxControlRegister = Pin20SwMuxControlRegister::instance();

bool Pin20SwMuxControlRegister::setMuxMode(const MuxMode mode) const
{
    return SwMuxControlRegister::setMuxMode((uint32_t) mode);
}

//==============================================================================

Pin21SwMuxControlRegister Pin21SwMuxControlRegister::instance_;

Pin21SwMuxControlRegister &Pin21SwMuxControlRegister::instance()
{
    return instance_;
}

Pin21SwMuxControlRegister &Pin21SwMuxControlRegister = Pin21SwMuxControlRegister::instance();

bool Pin21SwMuxControlRegister::setMuxMode(const MuxMode mode) const
{
    return SwMuxControlRegister::setMuxMode((uint32_t) mode);
}

//==============================================================================

Pin23SwMuxControlRegister Pin23SwMuxControlRegister::instance_;

Pin23SwMuxControlRegister &Pin23SwMuxControlRegister::instance()
{
    return instance_;
}

Pin23SwMuxControlRegister &Pin23SwMuxControlRegister = Pin23SwMuxControlRegister::instance();

bool Pin23SwMuxControlRegister::setMuxMode(const MuxMode mode) const
{
    return SwMuxControlRegister::setMuxMode((uint32_t) mode);
}
