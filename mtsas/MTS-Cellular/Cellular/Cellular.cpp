#include "mbed.h"
#include "Cellular.h"
#include "MTSText.h"
#include "MTSLog.h"

using namespace mts;

bool Cellular::init(MTSBufferedIO* io)
{
    if (io == NULL) {
        return false;
    }
    this->io = io;

    return true;
}

bool Cellular::configureSignals(unsigned int DCD, unsigned int DTR, unsigned int RESET)
{
    //Set DCD - The radio will raise and lower this line
    if (DCD != NC) {
        dcd = new DigitalIn(PinName(DCD));
    }
    /* Set DTR - This line should be lowered when we want to talk to the radio and raised when we're done
    * for now we will lower it in the constructor and raise it in the destructor.
    */
    if (DTR != NC) {
        dtr = new DigitalOut(PinName(DTR));
        dtr->write(0);
    }
    //Set RESET - Set the hardware reset line to the radio
    if (RESET != NC) {
        resetLine = new DigitalOut(PinName(RESET));
    }
    return true;
}

std::string Cellular::getRegistrationNames(Registration registration)
{
    switch(registration) {
        case NOT_REGISTERED:
            return "NOT_REGISTERED";
        case REGISTERED:
            return "REGISTERED";
        case SEARCHING:
            return "SEARCHING";
        case DENIED:
            return "DENIED";
        case UNKNOWN:
            return "UNKNOWN";
        case ROAMING:
            return "ROAMING";
        default:
            return "UNKNOWN ENUM";
    }
}

std::string Cellular::getRadioNames(Radio radio) {
    switch(radio) {
        case MTSMC_H5:
            return "MTSMC-H5";
        case MTSMC_EV3:
            return "MTSMC-EV3";
        case MTSMC_G3:
            return "MTSMC-G3";
        case MTSMC_C2:
            return "MTSMC-C2";
        case MTSMC_H5_IP:
            return "MTSMC-H5-IP";
        case MTSMC_EV3_IP:
            return "MTSMC-EV3-IP";
        case MTSMC_C2_IP:
            return "MTSMC-C2-IP";
        case MTSMC_LAT1:
            return "MTSMC_LAT1";
        case MTSMC_LEU1:
            return "MTSMC_LEU1";
        case MTSMC_LVW2:
            return "MTSMC_LVW2";
        default:
            return "UNKNOWN ENUM";
    }
}

Code Cellular::test()
{
    int i = 0;
    while (sendBasicCommand("AT", 1000) != MTS_SUCCESS) {
        i++;
        if (i >= 30) {
            logError("Could not talk to radio after 30 tries");
            i = 0;
        }
        wait(1);
    }
    return MTS_SUCCESS;
}

int Cellular::getSignalStrength()
{
    string response = sendCommand("AT+CSQ", 1000);
    if (response.find("OK") == string::npos) {
        return -1;
    }
    int start = response.find(':');
    int stop = response.find(',', start);
    string signal = response.substr(start + 2, stop - start - 2);
    int value;
    sscanf(signal.c_str(), "%d", &value);
    return value;
}

Cellular::Registration Cellular::getRegistration()
{
    string response = sendCommand("AT+CREG?", 5000);
    if (response.find("OK") == string::npos) {
        return UNKNOWN;
    }
    int start = response.find(',');
    int stop = response.find(' ', start);
    string regStat = response.substr(start + 1, stop - start - 1);
    int value;
    sscanf(regStat.c_str(), "%d", &value);
    switch (value) {
        case 0:
            return NOT_REGISTERED;
        case 1:
            return REGISTERED;
        case 2:
            return SEARCHING;
        case 3:
            return DENIED;
        case 4:
            return UNKNOWN;
        case 5:
            return ROAMING;
    }
    return UNKNOWN;
}

Code Cellular::setDns(const std::string& primary, const std::string& secondary)
{
    return sendBasicCommand("AT#DNS=1," + primary + "," + secondary, 1000);
}

Code Cellular::sendSMS(const Sms& sms)
{
    return sendSMS(sms.phoneNumber, sms.message);
}

Code Cellular::sendBasicCommand(const std::string& command, unsigned int timeoutMillis, char esc)
{
    if(socketOpened) {
        logError("socket is open. Can not send AT commands");
        return MTS_ERROR;
    }

    string response = sendCommand(command, timeoutMillis, esc);
    if (response.size() == 0) {
        return MTS_NO_RESPONSE;
    } else if (response.find("OK") != string::npos) {
        return MTS_SUCCESS;
    } else if (response.find("ERROR") != string::npos) {
        return MTS_ERROR;
    } else {
        return MTS_FAILURE;
    }
}

string Cellular::sendCommand(const std::string& command, unsigned int timeoutMillis, char esc)
{
    if(io == NULL) {
        logError("MTSBufferedIO not set");
        return "";
    }
    if(socketOpened) {
        logError("socket is open. Can not send AT commands");
        return "";
    }

    io->rxClear();
    io->txClear();
    std::string result;

    //Attempt to write command
    if(io->write(command.data(), command.size(), timeoutMillis) != command.size()) {
        //Failed to write command
        if (command != "AT" && command != "at") {
            logError("failed to send command to radio within %d milliseconds", timeoutMillis);
        }
        return "";
    }

    //Send Escape Character
    if (esc != 0x00) {
        if(io->write(esc, timeoutMillis) != 1) {
            if (command != "AT" && command != "at") {
                logError("failed to send character '%c' (0x%02X) to radio within %d milliseconds", esc, esc, timeoutMillis);
            }
            return "";
        }
    }
    mbed::Timer tmr;
    char tmp[256];
    tmp[255] = 0;
    bool done = false;
    tmr.start();
    do {
        //Make a non-blocking read call by passing timeout of zero
        int size = io->read(tmp,255,0);    //1 less than allocated (timeout is instant)
        if(size > 0) {
            result.append(tmp, size);
        }
        
        //Check for a response to signify the completion of the AT command
        //OK, ERROR, CONNECT are the 3 most likely responses
        if(result.size() > (command.size() + 2)) {
                if(result.find("OK\r\n",command.size()) != std::string::npos) {
                    done = true;
                } else if (result.find("ERROR") != std::string::npos) {
                    done = true;
                } else if (result.find("NO CARRIER\r\n") != std::string::npos) {
                    done = true;
                }
                
                if(type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_EV3 || type == MTSMC_C2 || type == MTSMC_LAT1 || type == MTSMC_LEU1 || type == MTSMC_LVW2) {
                    if (result.find("CONNECT\r\n") != std::string::npos) {
                        done = true;
                    } 
                } else if (type == MTSMC_H5_IP || type == MTSMC_EV3_IP || type == MTSMC_C2_IP) {
                    if (result.find("Ok_Info_WaitingForData\r\n") != std::string::npos) {
                        done = true;
                    } else if (result.find("Ok_Info_SocketClosed\r\n") != std::string::npos) {
                        done = true;
                    } else if (result.find("Ok_Info_PPP\r\n") != std::string::npos) {
                        done = true;
                    } else if (result.find("Ok_Info_GprsActivation\r\n") != std::string::npos) {
                        done = true;
                    }
                }
        }
        
        if(tmr.read_ms() >= timeoutMillis) {
            if (command != "AT" && command != "at") {
                logWarning("sendCommand [%s] timed out after %d milliseconds", command.c_str(), timeoutMillis);
            }
            done = true;
        }
    } while (!done);
   
    return result;
}

Code Cellular::sendSMS(const std::string& phoneNumber, const std::string& message)
{
    string csmp;
    
    if (type == MTSMC_H5_IP || type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_LAT1 || type == MTSMC_LEU1) {
        csmp = "AT+CSMP=17,167,0,0";
    } else if (type == MTSMC_EV3_IP || type == MTSMC_EV3 || type == MTSMC_C2_IP || type == MTSMC_C2 || type == MTSMC_LVW2) {
        csmp = "AT+CSMP=,4098,0,2";
    } else {
        logError("unknown radio type [%d]", type);
        return MTS_FAILURE;
    }
    
    Code code = sendBasicCommand("AT+CMGF=1", 2000);
    if (code != MTS_SUCCESS) {
        logError("CMGF failed");
        return code;
    }
    
    code = sendBasicCommand(csmp, 1000);
    if (code != MTS_SUCCESS) {
        logError("CSMP failed [%s]", getRadioNames(type).c_str());
        return code;
    }
    
    string cmd = "AT+CMGS=\"";
    cmd.append("+");
    cmd.append(phoneNumber);
    cmd.append("\",145");
    for (int i = 0; i < 5; i++) {
        string response1 = sendCommand(cmd, 2000);
        if (response1.find('>') != string::npos) {
            break;
        }
        if (i >= 5) {
            logError("CMGS phone number failed");
            return MTS_NO_RESPONSE;
        }
        wait(1);
    }
    wait(.2);
    
    string  response2 = sendCommand(message, 15000, CTRL_Z);
    if (response2.find("+CMGS:") == string::npos) {
        logError("CMGS message failed");
        return MTS_FAILURE;
    }
        
    return MTS_SUCCESS;
}

std::vector<Cellular::Sms> Cellular::getReceivedSms()
{
    int smsNumber = 0;
    std::vector<Sms> vSms;
    std::string received;
    size_t pos;
    
    Code code = sendBasicCommand("AT+CMGF=1", 2000);
    if (code != MTS_SUCCESS) {
        logError("CMGF failed");
        return vSms;
    }
    
    received = sendCommand("AT+CMGL=\"ALL\"", 5000);
    pos = received.find("+CMGL: ");

    while (pos != std::string::npos) {
        Cellular::Sms sms;
        std::string line(Text::getLine(received, pos, pos));
        if(line.find("+CMGL: ") == std::string::npos) {
            continue;
        }
        //Start of SMS message
        std::vector<std::string> vSmsParts = Text::split(line, ',');
        if (type == MTSMC_H5_IP || type == MTSMC_H5 || type == MTSMC_G3 || type == MTSMC_LAT1 || type == MTSMC_LEU1) {
            /* format for H5 and H5-IP radios
             * <index>, <status>, <oa>, <alpha>, <scts>
             * scts contains a comma, so splitting on commas should give us 6 items
             */
            if(vSmsParts.size() != 6) {
                logWarning("Expected 5 commas. SMS[%d] DATA[%s]. Continuing ...", smsNumber, line.c_str());
                continue;
            }

            sms.phoneNumber = vSmsParts[2];
            sms.timestamp = vSmsParts[4] + ", " + vSmsParts[5];
        } else if (type == MTSMC_EV3_IP || type == MTSMC_EV3 || type == MTSMC_C2_IP || type == MTSMC_C2 || type == MTSMC_LVW2) {
            /* format for EV3 and EV3-IP radios
             * <index>, <status>, <oa>, <callback>, <date>
             * splitting on commas should give us 5 items
             */
            if(vSmsParts.size() != 5) {
                logWarning("Expected 4 commas. SMS[%d] DATA[%s]. Continuing ...", smsNumber, line.c_str());
                continue;
            }
            
            sms.phoneNumber = vSmsParts[2];
            /* timestamp is in a nasty format
             * YYYYMMDDHHMMSS
             * nobody wants to try and decipher that, so format it nicely
             * YY/MM/DD,HH:MM:SS
             */
            string s = vSmsParts[4];
            if (type == MTSMC_LVW2) {
                sms.timestamp = s.substr(3,2) + "/" + s.substr(5,2) + "/" + s.substr(7,2) + ", " + s.substr(9,2) + ":" + s.substr(11,2) + ":" + s.substr(13,2);
            } else {
                sms.timestamp = s.substr(2,2) + "/" + s.substr(4,2) + "/" + s.substr(6,2) + ", " + s.substr(8,2) + ":" + s.substr(10,2) + ":" + s.substr(12,2);
            }
        }

        if(pos == std::string::npos) {
            logWarning("Expected SMS body. SMS[%d]. Leaving ...", smsNumber);
            break;
        }
        //Check for the start of the next SMS message
        size_t bodyEnd = received.find("\r\n+CMGL:", pos);
        if(bodyEnd == std::string::npos) {
            //This must be the last SMS message
            bodyEnd = received.find("\r\n\r\nOK", pos);
        }
        //Safety check that we found the boundary of this current SMS message
        if(bodyEnd != std::string::npos) {
            sms.message = received.substr(pos, bodyEnd - pos);
        } else {
            sms.message = received.substr(pos);
            logWarning("Expected to find end of SMS list. SMS[%d] DATA[%s].", smsNumber, sms.message.c_str());
        }
        vSms.push_back(sms);
        pos = bodyEnd;
        smsNumber++;
    }
    logInfo("Received %d SMS", smsNumber);
    return vSms;
}

Code Cellular::deleteOnlyReceivedReadSms()
{
    return sendBasicCommand("AT+CMGD=1,1", 1000);
}

Code Cellular::deleteAllReceivedSms()
{
    return sendBasicCommand("AT+CMGD=1,4", 1000);
}

unsigned int Cellular::readable()
{
    if(io == NULL) {
        logWarning("MTSBufferedIO not set");
        return 0;
    }
    if(!socketOpened && !io->readable()) {
        logWarning("Socket is not open");
        return 0;
    }
    return io->readable();
}

std::string Cellular::getEquipmentIdentifier()
{
    string equipmentIdentifier = sendCommand("AT+CGSN", 2000);
    std::vector<std::string> lines = Text::split(equipmentIdentifier, "\r\n");
    
    if (equipmentIdentifier.find("OK") != string::npos) {                
        equipmentIdentifier = lines[1];
    } else {
        //Empty string signifies failure
        equipmentIdentifier.clear();
    }
    
    return equipmentIdentifier;
}

int Cellular::getRadioType()
{
    return type;
}

std::string Cellular::getRadioTypeString()
{
    return getRadioNames(type);
}

unsigned int Cellular::writeable()
{
    if(io == NULL) {
        logWarning("MTSBufferedIO not set");
        return 0;
    }
    if(!socketOpened) {
        logWarning("Socket is not open");
        return 0;
    }

    return io->writeable();
}

bool Cellular::setDeviceIP(std::string address)
{
    if (address.compare("DHCP") == 0) {
        return true;
    } else {
        logWarning("Radio does not support static IPs, using DHCP.");
        return false;
    }
}

std::string Cellular::getDeviceIP()
{
    return local_address;
}

//Turns off echo when it receives a true, turns on when it receives false
Code Cellular::echo(bool state)
{
    Code code;
    if (state) {
        code = sendBasicCommand("ATE0", 1000);
        echoMode = (code == MTS_SUCCESS) ? false : echoMode;
    } else {
        code = sendBasicCommand("ATE1", 1000);
        echoMode = (code == MTS_SUCCESS) ? true : echoMode;
    }
    return code;
}

//Pass 1 to enable socket closeable
//Pass 0 to disable socket closeable
Code Cellular::setSocketCloseable(bool enabled)
{
    if(socketCloseable == enabled) {
        return MTS_SUCCESS;
    }

    if(socketOpened) {
        logError("socket is already opened. Can not set closeable");
        return MTS_ERROR;
    }

    socketCloseable = enabled;

    return MTS_SUCCESS;
}

bool Cellular::isOpen()
{
    if(io->readable()) {
        logDebug("Assuming open, data available to read.");
        return true;
    }
    return socketOpened;
}

//Binds the socket to a specific port if able
bool Cellular::bind(unsigned int port)
{
    if(socketOpened) {
        logError("socket is open. Can not set local port");
        return false;
    }
    if(port > 65535) {
        logError("port out of range (0-65535)");
        return false;
    }
    local_port = port;
    return true;
}

bool Cellular::GPSenable(){
    return true;
}

bool Cellular::GPSdisable(){
    return true;
}

bool Cellular::GPSenabled(){
    return true;
}
Cellular::gpsData Cellular::GPSgetPosition(){
    gpsData response;
    response.success = true;
    return response;
}

bool Cellular::GPSgotFix(){
    return true;    
}