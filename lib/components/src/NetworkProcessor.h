#ifndef NETWORKPROCESSOR_H
#define NETWORKPROCESSOR_H

#include <QNEthernetUDP.h>

class NetworkProcessor
{
public:
    virtual ~NetworkProcessor() = default;

    virtual void connect() = 0;
protected:
    qindesign::network::EthernetUDP socket{4};
};

#endif //NETWORKPROCESSOR_H
