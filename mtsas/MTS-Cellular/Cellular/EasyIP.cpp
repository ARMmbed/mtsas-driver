#include "mbed.h"
#include "EasyIP.h"
#include "MTSText.h"
#include "MTSLog.h"
#include "CellUtils.h"
#include <string>

using namespace mts;

EasyIP::EasyIP(Radio type)
{
    this->type = type;
    io = NULL;
    dcd = NULL;
    dtr = NULL;
    resetLine = NULL;
    echoMode = true;
    gpsEnabled = false;
    pppConnected = false;
    socketMode = TCP;
    socketOpened = false;
    socketCloseable = true;
    local_port = 0;
    local_address = "";
    host_port = 0;
}

EasyIP::~EasyIP()
{
    if (dtr != NULL) {
        dtr->write(1);
    }
    
    delete dcd;
    delete dtr;
    delete resetLine;
}

//Initializes the MTS IO Buffer
bool EasyIP::init(MTSBufferedIO* io)
{
    char buf[128];

    if (! Cellular::init(io)) {
        return false;
    }

    logDebug("radio type: %s", Cellular::getRadioNames(type).c_str());
    //Turns on the HW flow control
    if(sendBasicCommand("AT+IFC=2,2", 2000) != MTS_SUCCESS) {
        logWarning("Failed to enable serial flow control");
    }
    // Shorten data sending timeout from 5s to 100ms
    // Some servers won't handle a timeout that long
    snprintf(buf, sizeof(buf), "AT#SCFG=1,%d,300,90,600,1", type == MTSMC_LVW2 ? 3 : 1);
    if (sendBasicCommand(string(buf), 2000) != MTS_SUCCESS) {
        logWarning("Failed to reconfigure socket timeout parameters");
    }
    return true;
}

bool EasyIP::connect()
{
    //Check if APN is not set, if it is not, connect will not work.
    if (type == MTSMC_H5_IP || type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_LAT1 || type == MTSMC_LEU1) {
        if(apn.size() == 0) {
            logDebug("APN is not set");
            return false;
        }
    }
    
    //Check if socket is open
    if(socketOpened) {
        return true;
    }

    //Check if already connected
    if(isConnected()) {
        return true;
    }
    
    Timer tmr;
    //Check Registration: AT+CREG? == 0,1
    tmr.start();
    do {
        Registration registration = getRegistration();
        if(registration != REGISTERED && registration != ROAMING) {
            logTrace("Not Registered [%d] ... waiting", (int)registration);
            wait(1);
        } else {
            break;
        }
    } while(tmr.read() < 30); 
    
    //Check RSSI: AT+CSQ
    tmr.reset();
    do {
        int rssi = getSignalStrength();
        logDebug("Signal strength: %d", rssi);
        if(rssi == 99 || rssi == -1) {
            logTrace("No Signal ... waiting");
            wait(1);
        } else {
            break;
        }
    } while(tmr.read() < 30);

    //Make PPP connection
    if (type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_LAT1 || type == MTSMC_LEU1) {
        logDebug("Making PPP Connection Attempt. APN[%s]", apn.c_str());
    } else {
        logDebug("Making PPP Connection Attempt");
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "AT#SGACT=%d,1", type == MTSMC_LVW2 ? 3 : 1);
    std::string pppResult = sendCommand(string(buf), 15000);
    std::vector<std::string> parts;
    if(pppResult.find("OK") != std::string::npos) {
        parts = Text::split(pppResult, "\r\n");
        if(parts.size() >= 2) {
            parts = Text::split(parts[1], " ");
            if (parts.size() >= 2) {
                local_address = parts[1];
            }
        }
        logInfo("PPP Connection Established: IP[%s]", local_address.c_str());
        pppConnected = true;

    } else {
        snprintf(buf, sizeof(buf), "%d,1", type == MTSMC_LVW2 ? 3 : 1);
        pppResult = sendCommand("AT#SGACT?", 2000);
        if(pppResult.find(string(buf)) != std::string::npos) {
           logDebug("Radio is already connected");
           pppConnected = true;
        } else {
            logError("PPP connection attempt failed");
            pppConnected = false;
        }
    }

    return pppConnected;
}

void EasyIP::disconnect()
{
    //AT#SGACT=1,0: Close PPP connection
    logDebug("Closing PPP Connection"); 
    std::string result;
    Timer tmr;
       
    if(socketOpened) {
        close(true);
    }
    
    //Sends AT#SGACT=1,0 command
    for (int y = 0; y < 5; y++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "AT#SGACT=%d,0", type == MTSMC_LVW2 ? 3 : 1);
        Code code = sendBasicCommand(string(buf), 1000);
        if (code == MTS_SUCCESS) {
            logDebug("Successfully closed PPP Connection");
            break;
        }
    }
    
    /* Ensure PPP link is down, else ping commands will put radio in unknown state
     * (Link is not immediate in disconnection, even though OK is returned)
     */
    tmr.start();
    while(tmr.read() < 30) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d,0", type == MTSMC_LVW2 ? 3 : 1);
        result = sendCommand("AT#SGACT?", 1000);
        if(result.find(string(buf)) != std::string::npos) {
            break;
        } else if(result.find("ERROR") != std::string::npos) {
            break;
        } else {
            wait(1);
        }
    }
    
    pppConnected = false;
    return;
}

bool EasyIP::isConnected()
{
    enum RadioState {IDLE, CONNECTING, CONNECTED, DISCONNECTED};
    //state flags for various connection components
    bool signal = false, regist = false, active = false;
    char buf[16];
    
    //1) Check if APN was set if we're on an HSPA radio
    if (type == MTSMC_H5_IP || type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_LAT1 || type == MTSMC_LEU1) {
        if(apn.size() == 0) {
            logDebug("APN is not set");
            return false;
        }
    }
    
    //2) Check that we do not have a live connection up
    if (socketOpened) {
        logDebug("Socket is opened");
        return true;
    }
    
    //3) Query the radio
    int rssi = getSignalStrength();
    if (rssi == 99 || rssi == -1) {
        //Signal strength is nonexistent
        signal = false;
    } else {
        signal = true;
    }
    
    Registration creg = getRegistration();
    if (creg == REGISTERED) {
        regist = true;
    } else {
        regist = false;
    }
    
    string reply = sendCommand("AT#SGACT?", 1000);
    snprintf(buf, sizeof(buf), "%d,1", type == MTSMC_LVW2 ? 3 : 1);
    if (reply.find(string(buf)) != std::string::npos) {
        active = true;
    } else {
        active = false;
    }
    
    //Updates pppConnected to reflect current connection state
    RadioState state;
    bool ppp = pppConnected;
    if (signal && regist && active) {
        state = CONNECTED;
        pppConnected = true;
    } else if (signal && !regist && !active) {
        state = IDLE;
        pppConnected = false;
    } else if (active) {
        state = CONNECTING;
    } else {
        state = DISCONNECTED;
        pppConnected = false;
    }
    
    //Compares current connection state with previous pppConnected variable state
    if (!ppp && state == CONNECTED) {
        logWarning("Internal PPP state tracking differs from radio (DISCONNECTED:CONNECTED)");
    } else if (ppp && state != CONNECTED) {
        string stateStr;
        switch (state) {
            case IDLE:
                stateStr = "IDLE";
                break;
            case CONNECTING:
                stateStr = "CONNECTING";
                break;
            case DISCONNECTED:
                stateStr = "DISCONNECTED";
                break;
            case CONNECTED:
                stateStr = "CONNECTED";
                break;
            default:
                stateStr = "UKNOWN";
                break;
        }
        logWarning("Internal PPP state tracking differs from radio (CONNECTED:%s)", stateStr.c_str());
    }
    
    return pppConnected;
}

void EasyIP::reset()
{
    disconnect();
    
    if(sendBasicCommand("AT#REBOOT", 10000) != MTS_SUCCESS) {
        logError("Socket Modem did not accept RESET command");
    } else {
        logWarning("Socket Modem is resetting, allow 30 seconds for it to come back");
        return;
    }
}

//Opens socket connection
bool EasyIP::open(const std::string& address, unsigned int port, Mode mode)
{
    char sOpenSocketCmd[256] = {0}; //String for AT command
    std::string sMode = "";
    int typeSocket = 0;
    int closeType = 0;
    
    //1) Check that we do not have a live connection up
    if(socketOpened) {
        //Check that the address, port, and mode match
        if(host_address != address || host_port != port || socketMode != mode) {
            if(socketMode == TCP) {
                logError("TCP socket already opened [%s:%d]", host_address.c_str(), host_port);
            } else {
                logError("UDP socket already opened [%s:%d]", host_address.c_str(), host_port);
            }
            return false;
        }

        logDebug("Socket already opened");
        return true;
    }

    //2) Check Parameters
    if(port > 65535) {
        logError("port out of range (0-65535)");
        return false;
    }
    
    if(type == MTSMC_EV3 || type == MTSMC_LAT1 || type == MTSMC_LEU1 || type == MTSMC_LVW2) {
        if(!local_port) {
            logDebug("Local port set to 1, port 0 not supported for %s", getRadioNames(type).c_str());
            local_port = 1;
        }
    }

    //3) Check PPP connection
    if(!isConnected()) {
        logError("PPP not established.  Attempting to connect");
        if(!connect()) {
            logError("PPP connection failed");
            return false;
        } else {
            logDebug("PPP connection established");
        }
    }
    
    //4) Set escape sequence to not be transmitted through socket
    if(sendBasicCommand("AT#SKIPESC=1", 2000) != MTS_SUCCESS) {
        logWarning("Failed to disable escape sequence transmission on data mode suspension");
    }
    
    if(mode == TCP) {
        typeSocket = 0;
        sMode = "TCP";
    } else {
        typeSocket = 1;
        sMode = "UDP";
    }
    
    if(socketCloseable) {
        closeType = 0;
    } else {
        closeType = 255;
    }
    
    //5) Open Socket  
    sprintf(sOpenSocketCmd, "AT#SD=1,%d,%d,\"%s\",%d,%d,0", typeSocket, port, address.c_str(), closeType, local_port);
    std::string response = sendCommand(sOpenSocketCmd, 60000);
    
    if(response.find("CONNECT") != std::string::npos) {
        host_address = address;
        host_port = port;
        
        logInfo("Opened %s Socket [%s:%d]", sMode.c_str(), address.c_str(), port);
        socketOpened = true;
        socketMode = mode;
    } else {
        logWarning("Unable to open %s Socket [%s:%d]", sMode.c_str(), address.c_str(), port);
        socketOpened = false;
    }
    
    return socketOpened;
}

//Closes socket connection
bool EasyIP::close(bool shutdown)
{
    
    if(io == NULL) {
        logError("MTSBufferedIO not set");
        return false;
    }

    if(!socketOpened) {
        logWarning("Socket close() called, but socket was not open");
        return true;
    }

    if(!socketCloseable) {
        logError("Socket is not closeable");
        return false;
    }
    
    if(!sendEscapeCommand()) {
        logError("Failed to exit online mode");
        return false;
    } else {
        socketOpened = false;
    }
    
    if (sendBasicCommand("AT#SH=1", 2000) != MTS_SUCCESS) {
        logDebug("Failed to close socket connection");
    }
    
    //Clear receive buffer
    if (shutdown) {
        int counter = 0;
        char tmp[256];
        do {
            if(socketOpened == false) {
                break;
            }
            read(tmp, 256, 1000);
        } while(counter++ < 10);
    
        io->rxClear();
        io->txClear();
    }

    return !socketOpened;
}

//Read from socket
int EasyIP::read(char* data, int max, int timeout)
{
    if(io == NULL) {
        logError("MTSBufferedIO not set");
        return -1;
    }

    //Check that nothing is in the rx buffer
    if(!socketOpened && !io->readable()) {
        logError("Socket is not open");
        return -1;
    }

    int bytesRead = 0;

    if(timeout >= 0) {
        bytesRead = io->read(data, max, static_cast<unsigned int>(timeout));
    } else {
        bytesRead = io->read(data, max);
    }
    
    //Scan for socket closed message
    if(bytesRead > 0 && socketCloseable) {
        for(int i = 0; i < bytesRead; i++) {
            if(data[i] == 'N') {
                if(strstr(&data[i], "NO CARRIER")) {
                    logTrace("Found socket closed message. Checking validity");
                    //Close socket and Cut Off End of Message
                    socketOpened = socketCheck(); //Verifies legitimacy of socket disconnect
                    if(socketOpened) {
                        logDebug("Socket still open");
                        continue;
                    } else {
                        logDebug("Socket closed");
                        data[i] = '\0';
                        bytesRead = i;
                        break;
                    }
                }
            }
        }
    }
    return bytesRead;
}

//Write to socket
int EasyIP::write(const char* data, int length, int timeout)
{
    if(io == NULL) {
        logError("MTSBufferedIO not set");
        return -1;
    }

    if(!socketOpened) {
        logError("Socket is not open");
        return -1;
    }

    int bytesWritten = 0;
    int size = length;
    int failedWrites = 0;
    if(timeout >= 0) {
        Timer tmr;
        tmr.start();
        do {
            int available = io->writeable();
            if (available > 0) {
                size = mts_min(available, length - bytesWritten);
                bytesWritten += io->write(&data[bytesWritten], size);
            } else {
                wait(0.05);
            }
        } while (tmr.read_ms() <= timeout && bytesWritten < length);
    } else {
        //If timeout is -1:
        do {
            int available = io->writeable();
            if(available > 0) {
                size = mts_min(available, length - bytesWritten);
                int currentWritten = io->write(&data[bytesWritten], size);
                bytesWritten += currentWritten;
                if(!currentWritten) {
                    failedWrites++;
                }
                if(failedWrites > 10) {
                    logError("Couldn't write any characters");
                    return bytesWritten;
                }
            } else {
                wait(0.05);
            }
        } while (bytesWritten < length); 
    }
    return bytesWritten;
}

Code EasyIP::setApn(const std::string& apn)
{
    if (type == MTSMC_H5 || type == MTSMC_G3) {
         //CGDCONT has options: IP,PPP,IPv6
        Code code = sendBasicCommand("AT+CGDCONT=1,IP," + apn, 1000);
        if (code != MTS_SUCCESS) {
            return code;
        }
        this->apn = apn;
        return code;
    } else if (type == MTSMC_LAT1 || type == MTSMC_LEU1) {
         //CGDCONT has options: IP,PPP,IPv6
        Code code = sendBasicCommand("AT+CGDCONT=1,\"IP\",\"" + apn + "\"", 1000);
        if (code != MTS_SUCCESS) {
            return code;
        }
        this->apn = apn;
        return code;
    } else {
        logInfo("CDMA radios don't need an APN");
        return MTS_SUCCESS;
    }
}

bool EasyIP::ping(const std::string& address)
{
    char buffer[256] = {0};
    std::vector<std::string> parts;
    int TTL=0;
    int Timeout=0;
    
    //Format parameters for sending to radio
    sprintf(buffer, "AT#PING=%s,1,32,%d", address.c_str(), (PINGDELAY*10));
    
    for(int pngs=0; pngs<PINGNUM; pngs++) {
        std::string response = sendCommand(buffer, (PINGDELAY*2000)); //Send 1 ping
        if(response.empty() || response.find("ERROR") != std::string::npos) {
            continue; //Skip current loop if send command fails
        }
        parts = Text::split(response, "\r\n");
        if(parts.size() < 2) {
            continue;
        }
        parts = Text::split(parts[1], ",");
        if(parts.size() < 4) {
            continue;
        }
        //Parse TTL and Timeout values
        Timeout = std::atoi(parts[2].c_str());
        TTL = std::atoi(parts[3].c_str());
                
        if((Timeout < 600) && (TTL < 255)) {
            return true;
        }
    }   
    return false;
}

bool EasyIP::sendEscapeCommand()
{
    //string Cellular::sendCommand(const std::string& command, unsigned int timeoutMillis, char esc)
    if(io == NULL) {
        logError("MTSBufferedIO not set");
        return false;
    }
    if(!socketOpened) {
        logError("Socket is not open.");
        return false;
    }
    
    if(!socketCloseable) {
        logError("Socket is not closeable");
        return false;
    }
    
    io->rxClear();
    io->txClear();
    
    std::string result;
    unsigned int timeoutMillis = 10000;
    //Attempt to write command
    //Format for +++ command is 1 second wait, send +++, then another second wait
    wait(1.2); 
    if(io->write("+++", 3, timeoutMillis) != 3) {
        //Failed to write command
        logError("failed to send command to radio within %d milliseconds", timeoutMillis);
        return false;
    }
    
    Timer tmr;
    char tmp[256];
    tmp[255] = 0;
    bool done = false;
    bool exitmode = false;
    tmr.start();
    do {
        //Make a non-blocking read call by passing timeout of zero
        int size = io->read(tmp,255,0);    //1 less than allocated (timeout is instant)
        if(size > 0) {
            result.append(tmp, size);
        }
        if(result.find("OK\r\n") != std::string::npos) {
            exitmode = true;
            done = true;
        } else if(result.find("NO CARRIER\r\n") != std::string::npos) {
            socketOpened = false;
            exitmode = true;
            done = true;
        } else if(result.find("ERROR") != std::string::npos) {
            exitmode = false;
            done = true;
        }
        if(tmr.read_ms() >= timeoutMillis) {
            logDebug("Escape sequence [+++] timed out after %d milliseconds", timeoutMillis);
            exitmode = true;
            done = true;
        }
    } while (!done);
    
    wait(0.1); //Without a slight wait time after receiving OK, radio would
               //fail to correctly receive and respond to the next AT command
    return exitmode;
}

bool EasyIP::socketCheck() {
    enum SocketStatus {SOCKETCLOSED = 0, SOCKETACTIVEDATA = 1, SOCKETSUSPEND = 2, SOCKETSUSPENDDATA = 3, SOCKETLISTEN = 4, SOCKETINCOMING = 5, ERROR = 9};
    bool status = false;
    int socketInfo = ERROR;
    std::vector<std::string> params;
    
    //Goes from data mode to command mode
    if(sendEscapeCommand()) {
        std::string reply = sendCommand("AT#SS=1", 2000);
        if(reply.find("OK") != std::string::npos) {
            //Found valid response
            params = Text::split(reply, "\r\n");
            params = Text::split(params[1], ",");
            socketInfo = atoi(params[1].c_str());
        } else {
            logError("Could not determine socket status[%d]",socketInfo);
            socketInfo = ERROR;
        }
    } else {
        status = false; //Return value of socketOpened when checking
    }
    
    //Check socket status query
    if(socketInfo == SOCKETINCOMING || socketInfo == SOCKETACTIVEDATA || socketInfo == SOCKETSUSPEND || socketInfo == SOCKETSUSPENDDATA || socketInfo == SOCKETLISTEN) { //Socket opened responses
        status = true;
    } else if(socketInfo == SOCKETCLOSED || socketInfo == SOCKETINCOMING) {
        status = false;
    } else {
        logError("Could not determine socket status[%d]",socketInfo);
        status = false;
    }
    
    //Reconnect to active socket if able
    if(status) {
        std::string reconnect = sendCommand("AT#SO=1", 2000);
        if(reconnect.find("CONNECT") != std::string::npos || reconnect.find("OK") != std::string::npos) {
        } else {
            logError("Failed to resume socket connection");
        }
    }
    return status;
}

bool EasyIP::GPSenable() {
//The HE910 returns an ERROR if you try to enable when it is already enabled.
// That's why we need to check if GPS is enabled before enabling it.
    if(GPSenabled()) {
        logInfo("GPS was already enabled.");
        return true;
    }
//The LE910-NAG requires AT$GPSSLSR=2,3 to enable GPS but can use AT$GPSP=0 to disable it.    
    if(type == MTSMC_LAT1){
        Code code = sendBasicCommand("AT$GPSSLSR=2,3", 2000);
        if (code == MTS_SUCCESS) {
            gpsEnabled = true;        
            logInfo("GPS enabled.");
            return true;
        } else {
            logError("Enable GPS failed!");        
            return false;
        }        
    } else {
        Code code = sendBasicCommand("AT$GPSP=1", 2000);
        if (code == MTS_SUCCESS) {
            gpsEnabled = true;        
            logInfo("GPS enabled.");
            return true;
        } else {
            logError("Enable GPS failed.");
            return false;
        }
    }
}

bool EasyIP::GPSdisable() {
// The HE910 returns an ERROR if you try to disable when it is already disabled.
// That's why we need to check if GPS is disabled before disabling it.
    if(!GPSenabled()) {
        logInfo("GPS was already disabled.");
        return true;
    }
    Code code = sendBasicCommand("AT$GPSP=0", 2000);
    if (code == MTS_SUCCESS) {
        gpsEnabled = false;        
        logInfo("GPS disabled.");
        return true;
    } else {
        logError("Disable GPS failed.");
        return false;
    }
}

bool EasyIP::GPSenabled() {
    std::string reply = sendCommand("AT$GPSP?", 1000);
    if(reply.find("1") != std::string::npos) {
        gpsEnabled = true;
        return true;
    } else {
        gpsEnabled = false;
        return false;
    }
}

Cellular::gpsData EasyIP::GPSgetPosition(){
    enum gpsFields{time, latitude, longitude, hdop, altitude, fix, cog, kmhr, knots, date, satellites, numOfFields };
    Cellular::gpsData position;
    if(!gpsEnabled) {
        logError("GPS is disabled... can't get position.");
        position.success = false;
        return position;
    }
    // Get the position information in string format.
    std::string gps = sendCommand("AT$GPSACP?", 1000);
    if(gps.find("OK") != std::string::npos) {
        position.success = true;
        // Remove echoed AT$GPSACP and leading non position characters.
        gps.erase(0,22);
        // Remove trailing CR/LF, CR/LF, OK and CR/LF.
        gps.erase(gps.end()-8, gps.end());
        // Split remaining data and load into corresponding structure fields.
        std::vector<std::string> gpsParts = Text::split(gps, ',');
        // Check size.
        if(gpsParts.size() != numOfFields) {
            logError("Expected %d fields but there are %d fields in \"%s\"", numOfFields, gpsParts.size(), gps.c_str());
            position.success = false;
            return position; 
        }
        position.latitude = gpsParts[latitude];
        position.longitude = gpsParts[longitude];
        position.hdop = atof(gpsParts[hdop].c_str());
        position.altitude = atof(gpsParts[altitude].c_str());
        position.fix = atoi(gpsParts[fix].c_str());
        position.cog = gpsParts[cog];
        position.kmhr = atof(gpsParts[kmhr].c_str());
        position.knots = atof(gpsParts[knots].c_str());
        position.satellites = atoi(gpsParts[satellites].c_str());
        if((gpsParts[date].size() == 6) && (gpsParts[time].size() == 10)) {
            position.timestamp = gpsParts[date].substr(4,2) + "/" + gpsParts[date].substr(2,2) + 
            "/" + gpsParts[date].substr(0,2) + ", " + gpsParts[time].substr(0,2) + 
            ":" + gpsParts[time].substr(2,2) + ":" + gpsParts[time].substr(4,6);        
        }
        return position;     
    } else {
        position.success = false;
        logError("NO \"OK\" returned from GPS position command \"AT$GPSACP?\".");
        return position;
    }
}   
    
bool EasyIP::GPSgotFix() {
    if(!gpsEnabled) {
        logError("GPS is disabled... can't get fix.");
        return false;
    }
    Cellular::gpsData position = GPSgetPosition();
    if(!position.success) {
        return false;
    } else if(position.fix < 2){
        logWarning("No GPS fix. GPS fix can take a few minutes. Check GPS antenna attachment and placement.");
        return false;
    } else {
        logInfo("Got GPS fix.");
        return true;
    }
}