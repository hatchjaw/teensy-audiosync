#ifndef ETHERNETMANAGER_H
#define ETHERNETMANAGER_H

#include <NetworkProcessor.h>
#include <ProgramComponent.h>


class EthernetManager final : public ProgramComponent
{
protected:
    void beginImpl() override;

public:
    EthernetManager(const char *hostName, const std::vector<NetworkProcessor *> &networkProcessors);

    void run() override;

    size_t printTo(Print &p) const override;

private:
    std::vector<NetworkProcessor *> networkProcessors;

    uint8_t mac[6]{};
    IPAddress staticIP{192, 168, 10, 255};
    IPAddress subnetMask{255, 255, 255, 0};
    IPAddress gateway{192, 168, 10, 1};
    const char *hostName;
};


#endif //ETHERNETMANAGER_H
