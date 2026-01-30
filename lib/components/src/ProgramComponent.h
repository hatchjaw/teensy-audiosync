#ifndef PROGRAMCOMPONENT_H
#define PROGRAMCOMPONENT_H

#include <Printable.h>

class ProgramComponent : public Printable
{
protected:
    virtual void beginImpl() = 0;

public:
    virtual void begin() final
    {
        if (ready) return;
        beginImpl();
        ready = true;
    }

    virtual void run() = 0;

    virtual size_t printTo(Print &p) const = 0;

private:
    bool ready{false};
};

#endif //PROGRAMCOMPONENT_H
