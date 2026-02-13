#include "ComponentManager.h"
#include <Print.h>

ComponentManager::ComponentManager(const std::vector<ProgramComponent *> &components, const SystemUtils::LogLevel logLevel)
    : programComponents(components),
      logging(logLevel)
{
}

void ComponentManager::beginImpl()
{
    for (auto *c: programComponents) {
        c->begin();
    }
    Serial.println("=== Interrupt Priorities ===");
    Serial.printf("IRQ_DMA_CH0 (Audio): %d\n", NVIC_GET_PRIORITY(IRQ_DMA_CH0));
    Serial.printf("IRQ_ENET: %d\n", NVIC_GET_PRIORITY(IRQ_ENET));
    Serial.printf("IRQ_ENET_TIMER: %d\n", NVIC_GET_PRIORITY(IRQ_ENET_TIMER));
    Serial.printf("IRQ_SOFTWARE: %d\n", NVIC_GET_PRIORITY(IRQ_SOFTWARE));
}

void ComponentManager::run()
{
    for (auto *c: programComponents) {
        c->run();
    }

    if (elapsed > threshold) {
        elapsed = 0;
        if (logging > SystemUtils::None) {
            Serial.println(*this);
        }
    }
}

size_t ComponentManager::printTo(Print &p) const
{
    size_t total{0};
    total += p.println() +
            p.println("==============================================================================");
    for (const auto *c: programComponents) {
        total += p.print(*c);
    }
    total += p.println("==============================================================================");

    return total;
}
