#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H

#include <elapsedMillis.h>
#include <Printable.h>
#include <ProgramComponent.h>
#include <SystemUtils.h>
#include <vector>

class ComponentManager final : public ProgramComponent
{
public:
    explicit ComponentManager(const std::vector<ProgramComponent *> &components, SystemUtils::LogLevel logLevel = SystemUtils::None);

protected:
    void beginImpl() override;

public:
    void run() override;

    size_t printTo(Print &p) const override;

private:
    std::vector<ProgramComponent *> programComponents;
    elapsedMillis elapsed;
    static constexpr int threshold{1000};
    SystemUtils::LogLevel logging;
};


#endif //COMPONENTMANAGER_H
