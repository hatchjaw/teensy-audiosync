#include "EthernetManager.h"
#include <QNEthernet.h>
#include <SystemUtils.h>

EthernetManager::EthernetManager(const char *hostName, const std::vector<NetworkProcessor *> &networkProcessors)
    : networkProcessors(networkProcessors),
      hostName(hostName)
{
}

void EthernetManager::beginImpl()
{
    qindesign::network::Ethernet.setHostname("t41ptpsubscriber");
    qindesign::network::Ethernet.macAddress(mac);
    staticIP[2] = mac[4];
    staticIP[3] = mac[5];
    qindesign::network::Ethernet.begin(staticIP, subnetMask, gateway);
    qindesign::network::EthernetIEEE1588.begin();

    qindesign::network::Ethernet.onLinkState([this](const bool state)
    {
        Serial.printf("\n[Ethernet] Link %dMbps %s\n", qindesign::network::Ethernet.linkSpeed(), state ? "ON" : "OFF");
        if (state) {
            for (auto *np: networkProcessors) {
                np->connect();
            }
        }
    });

    Serial.printf("Mac address:   %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("IP:            ");
    Serial.println(qindesign::network::Ethernet.localIP());
    Serial.println();
}

void EthernetManager::run()
{
}

size_t EthernetManager::printTo(Print &p) const
{
    return p.print("IP: ") +
           p.print(qindesign::network::Ethernet.localIP()) +
           p.printf(" | MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) +
           p.printf(" | SN: %" PRIu32, SystemUtils::computeSerialNumber()) +
           p.println();
}
