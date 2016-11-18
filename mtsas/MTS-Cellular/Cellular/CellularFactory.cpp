#include "mbed.h"
#include "CellularFactory.h"
#include "MTSLog.h"
#include <string>

using namespace mts;

Cellular* CellularFactory::create(MTSBufferedIO* io) {
    bool uip;
    std::string model;
    std::string reply;
    Cellular::Radio type = Cellular::NA;
    Cellular* cell;
    
    /* wait for radio to get into a good state */
    while (true) {
        if (sendCommand(io, "AT", 1000).find("OK") != string::npos) {
            logTrace("radio replied");
            break;
        } else {
            logTrace("waiting on radio...");
        }
        wait(1);
    }
    
    while (true) {
        /* AT#VVERSION is a UIP specific AT command
         * if we get an error response, we're not using a UIP board */
        reply = sendCommand(io, "AT#VVERSION", 2000);
        if ((reply.find("ERROR") != string::npos) || (reply.find("error") != string::npos)) {
            uip = false;
            break;
        } else if (reply.find("VVERSION:") != string::npos) {
            uip = true;
            break;
        } else {
            logTrace("Checking for UIP chip");
        }
        wait(1);
    }
    
    /* "ATI4" gets us the model (HE910, DE910, etc) */
    while (true) {
        string mNumber;
        model = sendCommand(io, "ATI4", 3000);
        if (uip) {
            if (model.find("HE910") != string::npos) {
                type = Cellular::MTSMC_H5_IP;
                mNumber = "HE910";
            } else if (model.find("DE910") != string::npos) {
                type = Cellular::MTSMC_EV3_IP;
                mNumber = "DE910";
            } else if (model.find("CE910") != string::npos) {
                type = Cellular::MTSMC_C2_IP;
                mNumber = "CE910";
            }
            if (type != Cellular::NA) {
                cell = new UIP(type);
                logDebug("UIP radio model: %s", mNumber.c_str());
                break;
            }
        } else {
            if (model.find("HE910") != string::npos) {
                type = Cellular::MTSMC_H5;
                mNumber = "HE910";
            } else if (model.find("DE910") != string::npos) {
                type = Cellular::MTSMC_EV3;
                mNumber = "DE910";
            } else if (model.find("CE910") != string::npos) {
                type = Cellular::MTSMC_C2;
                mNumber = "CE910";
            } else if (model.find("GE910") != string::npos) {
                type = Cellular::MTSMC_G3;
                mNumber = "GE910";
            } else if (model.find("LE910-NAG") != string::npos) {
                type = Cellular::MTSMC_LAT1;
                mNumber = "LE910-NAG";
            } else if (model.find("LE910-SVG") != string::npos) {
                type = Cellular::MTSMC_LVW2;
                mNumber = "LE910-SVG";
            } else if (model.find("LE910-EUG") != string::npos) {
                type = Cellular::MTSMC_LEU1;
                mNumber = "LE910-EUG";
            }
            if (type != Cellular::NA) {
                cell = new EasyIP(type);
                logDebug("EasyIP radio model: %s", mNumber.c_str());
                break;
            }
        }
        logTrace("Determining radio type");
        wait(1);
    }

    if (! cell->init(io)) {
        logError("cellular initialization failed");
        return NULL;
    }

    return cell;
}
