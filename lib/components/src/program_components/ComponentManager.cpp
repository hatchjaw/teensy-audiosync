#include "ComponentManager.h"
#include <Print.h>

ComponentManager::ComponentManager(const std::vector<ProgramComponent *> &components)
    : programComponents(components)
{
}

void ComponentManager::beginImpl()
{
    for (auto *c: programComponents) {
        c->begin();
    }
}

void ComponentManager::run()
{
    for (auto *c: programComponents) {
        c->run();
    }
}

size_t ComponentManager::printTo(Print &p) const
{
    auto total{0};
    for (auto *c: programComponents) {
        total += p.print(*c);
    }
    return total;
}
