#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H

#include <Printable.h>
#include <ProgramComponent.h>
#include <vector>


class ComponentManager final : public ProgramComponent
{
public:
    explicit ComponentManager(const std::vector<ProgramComponent *> &components);

protected:
    void beginImpl() override;

public:
    void run() override;

    size_t printTo(Print &p) const override;

private:
    std::vector<ProgramComponent *> programComponents;
};


#endif //COMPONENTMANAGER_H
