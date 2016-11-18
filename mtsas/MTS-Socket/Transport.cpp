#include "Transport.h"

IPStack* Transport::transport = NULL;

void Transport::setTransport(IPStack* type)
{
    transport = type;
}

IPStack* Transport::getInstance()
{
    return transport;
}

