#ifndef PROGRAMCOMPONENT_H
#define PROGRAMCOMPONENT_H

class ProgramComponent
{
public:
    virtual ~ProgramComponent() = default;

    virtual void begin() = 0;

    virtual void run() = 0;
};

#endif //PROGRAMCOMPONENT_H
