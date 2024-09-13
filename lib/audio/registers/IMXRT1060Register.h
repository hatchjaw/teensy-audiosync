#ifndef IMXRT1060REGISTER_H
#define IMXRT1060REGISTER_H

#include <Arduino.h>

/**
 * Description of a 32-bit register for the i.MX RT1060 MCU
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
        return p.printf("%s: %X", m_Name, getValue());
    }

    /**
     * Get the value of the register
     * @return the value of the register.
     */
    uint32_t getValue() const
    {
        return *m_Address;
    }

protected:
    /**
     *
     * @param name A name to use when printing the value of this register.
     * @param address The address of the register, the appropriate offset of an
     * instance of IMXRT_REGISTER32_t.
     */
    IMXRT1060Register(const char *name, volatile uint32_t *address)
            : m_Name(name), m_Address(address) {}

    /**
     * Write to the register. The value of the register will be replaced with
     * `value`.
     * @param value
     */
    void write(uint32_t value) const { *m_Address = value; }

private:
    /**
     * The name of the register.
     */
    const char *m_Name;
    /**
     * The address of the register.
     */
    volatile uint32_t *m_Address;
};

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
              m_AddressTog(address + 3) {}

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
