#ifndef NETWORKPROCESSOR_H
#define NETWORKPROCESSOR_H

#include <QNEthernetUDP.h>

class NetworkProcessor
{
public:
    virtual void connect() = 0;
protected:
    qindesign::network::EthernetUDP socket;
};

#endif //NETWORKPROCESSOR_H
