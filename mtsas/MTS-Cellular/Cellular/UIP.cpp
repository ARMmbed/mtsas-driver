#include "mbed.h"
#include "UIP.h"
#include "MTSText.h"
#include "MTSLog.h"
#include "CellUtils.h"

using namespace mts;

UIP::UIP(Radio type)
{
    this->type = type;
    io = NULL;
    dcd = NULL;
    dtr = NULL;
    resetLine = NULL;
    echoMode = true;
    pppConnected = false;
    socketMode = TCP;
    socketOpened = false;
    socketCloseable = true;
    local_port = 0;
    local_address = "";
    host_port = 0;
}

UIP::~UIP()
{
    if (dtr != NULL) {
        dtr->write(1);
    }
    
    delete dcd;
    delete dtr;
    delete resetLine;
}

bool UIP::init(MTSBufferedIO* io)
{
    if (! Cellular::init(io)) {
        return false;
    }

    logDebug("radio type: %s", Cellular::getRadioNames(type).c_str());
    return true;
}

bool UIP::connect()
{
    //Check if APN is not set, if it is not, connect will not work.
    if (type == MTSMC_H5_IP || type == MTSMC_H5 || type == MTSMC_G3) {
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
        if(registration != REGISTERED) {
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

    //AT#CONNECTIONSTART: Make a PPP connection
    if (type == MTSMC_H5_IP) {
        logDebug("Making PPP Connection Attempt. APN[%s]", apn.c_str());
    } else {
        logDebug("Making PPP Connection Attempt");
    }
    std::string pppResult = sendCommand("AT#CONNECTIONSTART", 120000);
    
    if(pppResult.find("Ok_Info_GprsActivation") != std::string::npos) {
        std::vector<std::string> parts = Text::split(pppResult, "\r\n");
        if(parts.size() >= 2) {
            local_address = parts[1];
        }
        logInfo("PPP Connection Established: IP[%s]", local_address.c_str());
        pppConnected = true;

    } else {
        pppConnected = false;
    }

    return pppConnected;
}

void UIP::disconnect()
{
    //AT#CONNECTIONSTOP: Close a PPP connection
    logDebug("Closing PPP Connection");

    if(socketOpened) {
        close(true);
    }

    Code code = sendBasicCommand("AT#CONNECTIONSTOP", 10000);
    if(code == MTS_SUCCESS) {
        logDebug("Successfully closed PPP Connection");
    } else {
        logError("Closing PPP Connection [%d].  Continuing ...", (int)code);
    }

    pppConnected = false;
}

bool UIP::isConnected()
{
    //1) Check if APN was set if we're on an HSPA radio
    if (type == MTSMC_H5_IP || type == MTSMC_H5 || type == MTSMC_G3) {
        if(apn.size() == 0) {
            logDebug("APN is not set");
            return false;
        }
    }

    //2) Check that we do not have a live connection up
    if(socketOpened) {
        logDebug("Socket is opened");
        return true;
    }
    //3) Query the radio
    std::string result = sendCommand("AT#VSTATE", 3000);
    if(result.find("CONNECTED") != std::string::npos) {
        if(pppConnected == false) {
            logWarning("Internal PPP state tracking differs from radio (DISCONNECTED:CONNECTED)");
        }
        pppConnected = true;
    } else {
        if(pppConnected == true) {
            //Find out what state is
            size_t pos = result.find("STATE:");
            if(pos != std::string::npos) {
                result = Text::getLine(result, pos + sizeof("STATE:"), pos);
                logWarning("Internal PPP state tracking differs from radio (CONNECTED:%s)", result.c_str());
            } else {
                logError("Unable to parse radio state: [%s]", result.c_str());
            }

        }
        pppConnected = false;
    }

    return pppConnected;
}

bool UIP::open(const std::string& address, unsigned int port, Mode mode)
{
    char buffer[256] = {0};
    Code portCode, addressCode;
    
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

    //Set Local Port
    if(local_port != 0) {
        //Attempt to set local port
        sprintf(buffer, "AT#OUTPORT=%d", local_port);
        Code code = sendBasicCommand(buffer, 1000);
        if(code != MTS_SUCCESS) {
            logWarning("Unable to set local port (%d) [%d]", local_port, (int) code);
        }
    }

    //Set TCP/UDP parameters
    if(mode == TCP) {
        if(socketCloseable) {
            Code code = sendBasicCommand("AT#DLEMODE=1,1", 1000);
            if(code != MTS_SUCCESS) {
                logWarning("Unable to set socket closeable [%d]", (int) code);
            }
        }
        sprintf(buffer, "AT#TCPPORT=1,%d", port);
        portCode = sendBasicCommand(buffer, 1000);
        addressCode = sendBasicCommand("AT#TCPSERV=1,\"" + address + "\"", 1000);
    } else {
        if(socketCloseable) {
            Code code = sendBasicCommand("AT#UDPDLEMODE=1", 1000);
            if(code != MTS_SUCCESS) {
                logWarning("Unable to set socket closeable [%d]", (int) code);
            }
        }
        sprintf(buffer, "AT#UDPPORT=%d", port);
        portCode = sendBasicCommand(buffer, 1000);
        addressCode = sendBasicCommand("AT#UDPSERV=\"" + address + "\"", 1000);
    }

    if(portCode == MTS_SUCCESS) {
        host_port = port;
    } else {
        logError("Host port could not be set");
    }

    if(addressCode == MTS_SUCCESS) {
        host_address = address;
    } else {
        logError("Host address could not be set");
    }

    // Try and Connect
    std::string sMode;
    std::string sOpenSocketCmd;
    if(mode == TCP) {
        sOpenSocketCmd = "AT#OTCP=1";
        sMode = "TCP";
    } else {
        sOpenSocketCmd = "AT#OUDP";
        sMode = "UDP";
    }

    string response = sendCommand(sOpenSocketCmd, 30000);
    if (response.find("Ok_Info_WaitingForData") != string::npos) {
        logInfo("Opened %s Socket [%s:%d]", sMode.c_str(), address.c_str(), port);
        socketOpened = true;
        socketMode = mode;
    } else {
        logWarning("Unable to open %s Socket [%s:%d]", sMode.c_str(),  address.c_str(), port);
        socketOpened = false;
    }

    return socketOpened;
}

bool UIP::close(bool shutdown)
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

    if(io->write(ETX, 1000) != 1) {
        logError("Timed out attempting to close socket");
        return false;
    }

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

    socketOpened = false;
    return true;
}

int UIP::read(char* data, int max, int timeout)
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

    if(bytesRead > 0 && socketCloseable) {
        //Remove escape characters
        int index = 0;
        bool escapeFlag = false;
        for(int i = 0; i < bytesRead; i++) {
            if(data[i] == DLE || data[i] == ETX) {
                if(escapeFlag == true) {
                    //This character has been escaped
                    escapeFlag = false;
                } else if(data[bytesRead] == DLE) {
                    //Found escape character
                    escapeFlag = true;
                    continue;
                } else {
                    //ETX sent without escape -> Socket closed
                    logInfo("Read ETX character without DLE escape. Socket closed");
                    socketOpened = false;
                    continue;
                }
            }

            if(index != i) {
                data[index] = data[i];
            }
            index++;
        }
        bytesRead = index;
    }

    //Scan for socket closed message
    for(size_t i = 0; i < bytesRead; i++) {
        if(data[i] == 'O') {
            if(strstr(&data[i], "Ok_Info_SocketClosed")) {
                logInfo("Found socket closed message. Socket closed");
                //Close socket and Cut Off End of Message
                socketOpened = false;
                data[i] = '\0';
                bytesRead = i;
                break;
            }
        }
    }
    return bytesRead;
}

int UIP::write(const char* data, int length, int timeout)
{
    if(io == NULL) {
        logError("MTSBufferedIO not set");
        return -1;
    }

    if(!socketOpened) {
        logError("Socket is not open");
        return -1;
    }

    //In order to avoid allocating another buffer, capture indices of
    //characters to escape during write
    int specialWritten = 0;
    std::vector<int> vSpecial;
    if(socketCloseable) {
        for(int i = 0; i < length; i++) {
            if(data[i] == ETX || data[i] == DLE) {
                //Push back index of special characters
                vSpecial.push_back(i);
            }
        }
    }

    int bytesWritten = 0;
    if(timeout >= 0) {
        Timer tmr;
        tmr.start();
        do {
            int available = io->writeable();
            if (available > 0) {
                if(specialWritten < vSpecial.size()) {
                    //Check if current index is at a special character
                    if(bytesWritten == vSpecial[specialWritten]) {
                        if(available < 2) {
                            //Requires at least two bytes of space
                            wait(0.05);
                            continue;
                        }
                        //Ready to write special character
                        if(io->write(DLE)) {
                            specialWritten++;
                            if(io->write(data[bytesWritten])) {
                                bytesWritten++;
                            }
                        } else {
                            //Unable to write escape character, try again next round
                            wait(0.05);
                        }
                    } else {
                        //We want to write all the way up to the next special character
                        int relativeIndex = vSpecial[specialWritten] - bytesWritten;
                        int size = mts_min(available, relativeIndex);
                        bytesWritten += io->write(&data[bytesWritten], size);
                    }
                } else {
                    int size = mts_min(available, length - bytesWritten);
                    bytesWritten += io->write(&data[bytesWritten], size);
                }
            } else {
                wait(0.05);
            }
        } while (tmr.read_ms() <= timeout && bytesWritten < length);
    } else {
        for(int i = 0; i < vSpecial.size(); i++) {
            //Write up to the special character, then write the special character
            int size = vSpecial[i] - bytesWritten;
            int currentWritten = io->write(&data[bytesWritten], size);
            bytesWritten += currentWritten;
            if(currentWritten != size) {
                //Failed to write up to the special character.
                return bytesWritten;
            }
            if(io->write(DLE) && io->write(data[bytesWritten])) {
                bytesWritten++;
            } else {
                //Failed to write the special character.
                return bytesWritten;
            }
        }

        bytesWritten = io->write(&data[bytesWritten], length - bytesWritten);
    }

    return bytesWritten;
}

Code UIP::setApn(const std::string& apn)
{
    if (type == MTSMC_H5_IP) {
        Code code = sendBasicCommand("AT#APNSERV=\"" + apn + "\"", 1000);
        if (code != MTS_SUCCESS) {
            return code;
        }
        this->apn = apn;
        return code; //This will return MTS_SUCCESS
    } else {
        logInfo("CDMA radios don't need an APN");
        return MTS_SUCCESS;
    }
}

void UIP::reset()
{
    disconnect();
    Code code = sendBasicCommand("AT#RESET=0", 10000);
    if(code != MTS_SUCCESS) {
        logError("Socket Modem did not accept RESET command\n\r");
    } else {
        logWarning("Socket Modem is resetting, allow 30 seconds for it to come back\n\r");
    }
}

bool UIP::ping(const std::string& address)
{
    char buffer[256] = {0};
    Code code;

    code = sendBasicCommand("AT#PINGREMOTE=\"" + address + "\"", 1000);
    if (code != MTS_SUCCESS) {
        return false;
    }

    sprintf(buffer, "AT#PINGNUM=%d", 1);
    code = sendBasicCommand(buffer , 1000);
    if (code != MTS_SUCCESS) {
        return false;
    }

    sprintf(buffer, "AT#PINGDELAY=%d", PINGDELAY);
    code = sendBasicCommand(buffer , 1000);
    if (code != MTS_SUCCESS) {
        return false;
    }

    std::string response;
    for (int i = 0; i < PINGNUM; i++) {
        response = sendCommand("AT#PING", PINGDELAY * 2000);
        if (response.find("alive") != std::string::npos) {
            /* the "OK" that comes on a new line doesn't come right after this reply
             * wait a couple seconds to make sure we get it here, otherwise it could be read
             * as the response to the next AT command sent
             */
            wait(2);
            return true;
        }
    }
    return false;
}
