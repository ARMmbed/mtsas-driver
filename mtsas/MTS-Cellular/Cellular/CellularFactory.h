#ifndef CELLULARFACTORY_H
#define CELLULARFACTORY_H

#include "UIP.h"
#include "EasyIP.h"
#include "MTSBufferedIO.h"

namespace mts {

class CellularFactory
{
public:
    static Cellular* create(MTSBufferedIO* io);
};

}

#endif
