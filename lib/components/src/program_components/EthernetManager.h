#ifndef ETHERNETMANAGER_H
#define ETHERNETMANAGER_H

#include <NetworkProcessor.h>
#include <ProgramComponent.h>


class EthernetManager final : public ProgramComponent
{
protected:
    void beginImpl() override;

public:
    explicit EthernetManager(const std::vector<NetworkProcessor *> &networkProcessors);

    void run() override;

    uint32_t getSerialNumber() const;

    size_t printTo(Print &p) const override;

private:
    void computeSerialNumber();

    std::vector<NetworkProcessor *> networkProcessors;

    uint32_t serialNumber{};
    uint8_t mac[6]{};
    IPAddress staticIP{192, 168, 10, 255};
    IPAddress subnetMask{255, 255, 255, 0};
    IPAddress gateway{192, 168, 10, 1};
};


#endif //ETHERNETMANAGER_H
