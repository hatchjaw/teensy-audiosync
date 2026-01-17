#ifndef IMXRT1060REGISTER_H
#define IMXRT1060REGISTER_H

#include <Arduino.h>

/**
 * Represents a 32-bit register for the IMXRT1060 microcontroller.
 * Provides methods managing the value of the register.
 */
class IMXRT1060Register : public Printable
{
public:
    /**
      * Set up the register with a sensible default value.
      * @return true on success; false otherwise.
      */
    virtual bool begin() = 0;

    /**
     * Print the name and value of the register.
     * @param p An instance of a descendant of Print, to handle the printing
     * operation.
     * @return The number of characters that were printed.
     */
    size_t printTo(Print &p) const override
    {
        return p.printf("%s: %X", name, getValue());
    }

    /**
     * Get the value of the register
     * @return the value of the register.
     */
    uint32_t getValue() const
    {
        return *address;
    }

protected:
    /**
     *
     * @param registerName A name to use when printing the value of this
     * register.
     * @param registerAddress The address of the register, the appropriate
     * offset of an instance of IMXRT_REGISTER32_t.
     */
    IMXRT1060Register(const char *registerName, volatile uint32_t *registerAddress)
        : name(registerName), address(registerAddress)
    {
    }

    /**
     * Write to the register. The value of the register will be replaced with
     * the value provided.
     * @param value
     */
    void write(const uint32_t value) const { *address = value; }

    /**
     * Updates the value of a register by applying a bitmask.
     *
     * Conditionally modifies the current register value based on the provided bitmask.
     * If 'enable' is true, the bits specified in the mask are set.
     * If 'enable' is false, the bits specified in the mask are cleared.
     *
     * @param enable Determines whether to set or clear the bits defined by the mask.
     * @param mask The bitmask specifying the bits to modify in the register value.
     */
    void writeMask(const bool enable, const uint32_t mask) const
    {
        write(enable ? getValue() | mask : getValue() & ~mask);
    }

private:
    /**
     * The name of the register.
     */
    const char *name;
    /**
     * The address of the register.
     */
    volatile uint32_t *address;
};

//==============================================================================

/**
 * Description of a 32-bit register for the i.MX RT1060 MCU, with adjacent set,
 * clear, and toggle registers.
 */
class IMXRT1060BitbandRegister : public IMXRT1060Register
{
protected:
    /**
     * @inherit
     */
    IMXRT1060BitbandRegister(const char *name, volatile uint32_t *address)
        : IMXRT1060Register(name, address),
          m_AddressSet(address + 1),
          m_AddressClr(address + 2),
          m_AddressTog(address + 3)
    {
    }

    /**
     * Assign to the SET register. The bits of `mask` will be set; other bits
     * will not be modified.
     * @param mask
     */
    void set(uint32_t mask) const { *m_AddressSet = mask; }

    /**
     * Assign to the CLR register. The bits of `mask` will be cleared; other
     * bits will not be modified.
     * @param mask
     */
    void clear(uint32_t mask) const { *m_AddressClr = mask; }

    /**
     * Assign to the TOG register. The bits of `mask` will be toggled; other
     * bits will not be modified.
     * @param mask
     */
    void toggle(uint32_t mask) const { *m_AddressTog = mask; }

private:
    /**
     * Address of the SET register.
     */
    volatile uint32_t *m_AddressSet;
    /**
     * Address of the CLR register.
     */
    volatile uint32_t *m_AddressClr;
    /**
     * Address of the TOG register.
     */
    volatile uint32_t *m_AddressTog;
};

#endif //IMXRT1060REGISTER_H
