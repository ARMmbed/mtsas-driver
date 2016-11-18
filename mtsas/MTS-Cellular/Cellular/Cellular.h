#ifndef CELLULAR_H
#define CELLULAR_H

#include <string>
#include <vector>

#include "IPStack.h"
#include "MTSBufferedIO.h"
#include "CellUtils.h"

namespace mts
{

/** This is a class for communicating with a Cellular radio device.
* This class supports three main types of cellular radio interactions including:
* configuration and status processing, SMS processing, and TCP/UDP Socket
* data connections. This class also inherits from IPStack providing a common set of commands
* for communication devices that support IP protocols. It is also integrated with the standard
* mbed Sockets package and can therefore be used seamlessly with clients and services built
* on top of this interface already within the mbed library.
*
* For all examples below please change Pin Names to match your hardware configuration.
* It also assumes the use of RTS/CTS hardware handshaking using GPIOs. To disable this
* you will need to change settings on the radio module and and use the MTSSerial class
* instead of MTSSerialFlowControl.
*
* The following set of example code demonstrates how to send and receive configuration and
* status AT commands with the radio, create a data connection and test it:
* @code
* #include "mbed.h"
* #include "mtsas.h"
*
* int main(){
*    //Modify to match your apn if you are using an HSPA radio with a SIM card
*    const char APN[] = "";
*    
*    //Sets the log level to INFO, higher log levels produce more log output.
*    //Possible levels: NONE, FATAL, ERROR, WARNING, INFO, DEBUG, TRACE
*    MTSLog::setLogLevel(MTSLog::INFO_LEVEL);
*    
*    // STMicro Nucelo F401RE
*    // The supported jumper configurations of the MTSAS do not line up with
*    // the pin mapping of the Nucleo F401RE. Therefore, the MTSAS serial TX
*    // pin (JP8 Pin 2) must be manually jumped to Serial1 RX (Shield pin D2)
*    // and the MTSAS serial RX pin (JP9 Pin 2) pin must be manually jumped to
*    // Serial1 TX (Shield pin D8).
*    // Uncomment the following line to use the STMicro Nuceleo F401RE
*    //
*    MTSSerialFlowControl* io = new MTSSerialFlowControl(D8, D2, D3, D6);
*    
*    // Freescale KL46Z
*    // To configure the pins for the Freescale KL46Z board, use configuration B
*    // Uncomment the following line to use the Freescale KL46Z board
*    //
*    //MTSSerialFlowControl* io = new MTSSerialFlowControl(D2, D9, D3, D6);
*    
*    // Freescale K64F
*    // To configure the pins for the Freescale KL46Z board, use configuration A
*    // Uncomment the following line to use the Freescale KL46F board
*    //
*    //MTSSerialFlowControl* io = new MTSSerialFlowControl(D1, D0, D3, D6);
*    
*    //Sets the baud rate for communicating with the radio
*    io->baud(115200);
*    
*    //Create radio object
*    Cellular* radio = CellularFactory::create(io);
*    radio->configureSignals(D4,D7,RESET);
*    Transport::setTransport(radio);
*    
*    if (! radio) {
*        logFatal("Failed to initialize radio");
*        return 1;
*    }
*    
*    //Set radio APN
*    for (int i = 0; i < 10; i++) {
*        if (i >= 10) {
*            logError("Failed to set APN to %s", APN);
*        }
*        if (radio->setApn(APN) == MTS_SUCCESS) {
*            logInfo("Successfully set APN to %s", APN);
*            break;
*        } else {
*            wait(1);
*        }
*    }
*    
*    //Establish PPP link
*    for (int i = 0; i < 10; i++) {
*        if (i >= 10) {
*            logError("Failed to establish PPP link");
*        }
*        if (radio->connect() == true) {
*            logInfo("Successfully established PPP link");
*            break;
*        } else {
*            wait(1);
*        }
*    }
*    
*    //Ping google.com
*    for (int i = 0; i < 10; i++) {
*        if (i >= 10) {
*            logError("Failed to ping www.google.com");
*        }
*        if (radio->ping("www.google.com") == true) {
*            logInfo("Successfully pinged www.google.com");
*            break;
*        } else {
*            wait(1);
*        }
*    }
*    
*    //Disconnect ppp link
*    radio->disconnect();
*    
*    logInfo("End of example code");
*    return 0;
* }
*
* @endcode
*
* The following set of example code demonstrates how to process SMS messages:
* @code
* #include "mbed.h"
* #include "mtsas.h"
*
* int main(){
*    
*    //Sets the log level to INFO, higher log levels produce more log output.
*    //Possible levels: NONE, FATAL, ERROR, WARNING, INFO, DEBUG, TRACE
*    MTSLog::setLogLevel(MTSLog::INFO_LEVEL);
*    
*    //Modify to match your apn if you are using an HSPA radio with a SIM card
*    const char APN[] = "";
*    
*    //Phone number to send to and receive from. Must be in the form "1xxxxxxxxxx"
*    string PHONE_NUMBER = "";
*    
*    Cellular::Sms txtmsg;
*    txtmsg.phoneNumber = PHONE_NUMBER;
*    txtmsg.message = "Hello World! MTSAS is up and running!";
*    
*    // STMicro Nucelo F401RE
*    // The supported jumper configurations of the MTSAS do not line up with
*    // the pin mapping of the Nucleo F401RE. Therefore, the MTSAS serial TX
*    // pin (JP8 Pin 2) must be manually jumped to Serial1 RX (Shield pin D2)
*    // and the MTSAS serial RX pin (JP9 Pin 2) pin must be manually jumped to
*    // Serial1 TX (Shield pin D8).
*    // Uncomment the following line to use the STMicro Nuceleo F401RE
*    //
*    MTSSerialFlowControl* io = new MTSSerialFlowControl(D8, D2, D3, D6);
*    
*    // Freescale KL46Z
*    // To configure the pins for the Freescale KL46Z board, use configuration B
*    // Uncomment the following line to use the Freescale KL46Z board
*    //
*    //MTSSerialFlowControl* io = new MTSSerialFlowControl(D2, D9, D3, D6);
*    
*    // Freescale K64F
*    // To configure the pins for the Freescale KL46Z board, use configuration A
*    // Uncomment the following line to use the Freescale KL46F board
*    //
*    //MTSSerialFlowControl* io = new MTSSerialFlowControl(D1, D0, D3, D6);
*    
*    //Sets the baudrate for communicating with the radio
*    io->baud(115200); 
*    
*    //Creates a radio object
*    Cellular* radio = CellularFactory::create(io);
*    radio->configureSignals(D4,D7,RESET);
*    Transport::setTransport(radio);
*    
*    if (! radio) {
*        logFatal("Failed to initialize radio");
*        return 1;
*    }
*    
*    //Set radio APN
*    for (int i = 0; i < 10; i++) {
*        if (i >= 10) {
*            logError("Failed to set APN\n");
*        }
*        if (radio->setApn(APN) == MTS_SUCCESS) {
*            logInfo("Successfully set APN\n");
*            break;
*        } else {
*            wait(1);
*        }
*    }
*    
*    //Delete any previously received SMS messages
*    for (int i = 0; i < 10; i++) {
*        if (i >= 10) {
*            logError("Failed to delete SMS messages\n");
*        }
*        if (radio->deleteAllReceivedSms() == MTS_SUCCESS) {
*            logInfo("Deleted all SMS messages\n");
*            break;
*        } else {
*            wait(1);
*        }
*    }
*    
*    // Send SMS message to phone
*    for (int i = 1; i < 10; i++) {
*        if(radio->sendSMS(txtmsg) == MTS_SUCCESS) {
*            logInfo("Sent SMS successfully:<%s>\n", txtmsg.message.c_str());
*            break;
*        } else {
*            logError("Failed to send SMS<%s>\n", txtmsg.message.c_str());
*        }
*    }
*    
*    //Checking for received SMS messages
*    while (true) {
*        logInfo("Checking for received messages");
*        vector<Cellular::Sms> recv = radio->getReceivedSms();
*        if(recv.size() > 0) {
*            int size = recv.size();
*            for (int i = 0; i < size; i++) {
*                logInfo("Message %d: [%s] [%s] [%s]", i, recv[i].phoneNumber.c_str(), recv[i].timestamp.c_str(), recv[i].message.c_str());
*            }
*        }
*        
*        if(radio->deleteOnlyReceivedReadSms() != MTS_SUCCESS) {
*            logError("Failed to delete received and read SMS messages");
*        }
*        wait(10);
*    }
*    
*    logDebug("End of example code\n");
*    return 0;
* }
* @endcode
*/

class Cellular : public IPStack
{
public:
    // Class ping paramter constants
    static const unsigned int PINGDELAY = 3; //Time to wait on each ping for a response before timimg out (seconds)
    static const unsigned int PINGNUM = 4; //Number of pings to try on ping command

    /// Enumeration for different cellular radio types.
    enum Radio {
        NA, MTSMC_H5, MTSMC_EV3, MTSMC_G3, MTSMC_C2, MTSMC_H5_IP, MTSMC_EV3_IP, MTSMC_C2_IP, MTSMC_LAT1, MTSMC_LVW2, MTSMC_LEU1
    };

    /// An enumeration of radio registration states with a cell tower.
    enum Registration {
        NOT_REGISTERED, REGISTERED, SEARCHING, DENIED, UNKNOWN, ROAMING
    };

    /** This structure contains the data for an SMS message.
    */
    struct Sms {
        /// Message Phone Number
        std::string phoneNumber;
        /// Message Body
        std::string message;
        /// Message Timestamp
        std::string timestamp;
    };

    /** This structure contains the data for GPS position.
    */
    struct gpsData {
        bool success;
        /// Format is ddmm.mmmm N/S. Where: dd - degrees 00..90; mm.mmmm - minutes 00.0000..59.9999; N/S: North/South.
        std::string latitude;
        /// Format is dddmm.mmmm E/W. Where: ddd - degrees 000..180; mm.mmmm - minutes 00.0000..59.9999; E/W: East/West.
        std::string longitude;
        /// Horizontal Diluition of Precision.
        float hdop;
        /// Altitude - mean-sea-level (geoid) in meters.
        float altitude;
        /// 0 or 1 - Invalid Fix; 2 - 2D fix; 3 - 3D fix.
        int fix;
        /// Format is ddd.mm - Course over Ground. Where: ddd - degrees 000..360; mm - minutes 00..59.
        std::string cog;
        /// Speed over ground (Km/hr).
        float kmhr;
        /// Speed over ground (knots).
        float knots;
        /// Total number of satellites in use.
        int satellites;
        /// Date and time in the format YY/MM/DD,HH:MM:SS.
        std::string timestamp;
    };
    
    /** This method initializes the object with the underlying radio
    * interface to use. Note that this function MUST be called before
    * any other calls will function correctly on a Cellular object. Also
    * note that MTSBufferedIO is abstract, so you must use one of
    * its inherited classes like MTSSerial, MTSSerialFlowControl or write a class
    * similar to MTSSerialFlowControl which maps the MTSBufferedIO API
    * to your favorite serial library.
    *
    * @param io the io interface that is attached to the cellular radio.
    * The default is not connected.
    * @returns true if the init was successful, otherwise false.
    */
    virtual bool init(MTSBufferedIO* io);
    
    /** Sets up the physical connection pins
    *   (DTR,DCD, and RESET)
    */
    bool configureSignals(unsigned int DCD = NC, unsigned int DTR = NC, unsigned int RESET = NC);

    /** A method for testing command access to the radio.  This method sends the
    * command "AT" to the radio, which is a standard radio test to see if you
    * have command access to the radio.  The function returns when it receives
    * the expected response from the radio.
    *
    * @returns the standard AT Code enumeration.
    */
    virtual Code test();

    /** A method for getting the signal strength of the radio. This method allows you to
    * get a value that maps to signal strength in dBm. Here 0-1 is Poor, 2-9 is Marginal,
    * 10-14 is Ok, 15-19 is Good, and 20+ is Excellent.  If you get a result of 99 the
    * signal strength is not known or not detectable.
    *
    * @returns an integer representing the signal strength.
    */
    virtual int getSignalStrength();

    /** This method is used to check the registration state of the radio with the cell tower.
    * If not appropriatley registered with the tower you cannot make a cellular connection.
    *
    * @returns the registration state as an enumeration type.
    */
    virtual Registration getRegistration();

    /** This method is used to set the radios APN if using a SIM card. Note that the APN
    * must be set correctly before you can make a data connection. The APN for your SIM
    * can be obtained by contacting your cellular service provider.
    *
    * @param the APN as a string.
    * @returns the standard AT Code enumeration.
    */
    virtual Code setApn(const std::string& apn) = 0;

    /** This method is used to set the DNS which enables the use of URLs instead
    * of IP addresses when making a socket connection.
    *
    * @param the DNS server address as a string in form xxx.xxx.xxx.xxx.
    * @returns the standard AT Code enumeration.
    */
    virtual Code setDns(const std::string& primary, const std::string& secondary = "0.0.0.0");

    /** This method is used to send an SMS message. Note that you cannot send an
    * SMS message and have a data connection open at the same time.
    *
    * @param phoneNumber the phone number to send the message to as a string.
    * @param message the text message to be sent.
    * @returns the standard AT Code enumeration.
    */
    virtual Code sendSMS(const std::string& phoneNumber, const std::string& message);

    /** This method is used to send an SMS message. Note that you cannot send an
    * SMS message and have a data connection open at the same time.
    *
    * @param sms an Sms struct that contains all SMS transaction information.
    * @returns the standard AT Code enumeration.
    */
    virtual Code sendSMS(const Sms& sms);

    /** This method retrieves all of the SMS messages currently available for
    * this phone number.
    *
    * @returns a vector of existing SMS messages each as an Sms struct.
    */
    virtual std::vector<Cellular::Sms> getReceivedSms();

    /** This method can be used to remove/delete all received SMS messages
    * even if they have never been retrieved or read.
    *
    * @returns the standard AT Code enumeration.
    */
    virtual Code deleteAllReceivedSms();

    /** This method can be used to remove/delete all received SMS messages
    * that have been retrieved by the user through the getReceivedSms method.
    * Messages that have not been retrieved yet will be unaffected.
    *
    * @returns the standard AT Code enumeration.
    */
    virtual Code deleteOnlyReceivedReadSms();

    //Cellular Radio Specific
    /** A method for sending a generic AT command to the radio. Note that you cannot
    * send commands and have a data connection at the same time.
    *
    * @param command the command to send to the radio without the escape character.
    * @param timeoutMillis the time in millis to wait for a response before returning.
    * @param esc escape character to add at the end of the command, defaults to
    * carriage return (CR).  Does not append any character if esc == 0.
    * @returns all data received from the radio after the command as a string.
    */
    virtual std::string sendCommand(const std::string& command, unsigned int timeoutMillis, char esc = CR);

    /** A method for sending a basic AT command to the radio. A basic AT command is
    * one that simply has a response of either OK or ERROR without any other information.
    * Note that you cannot send commands and have a data connection at the same time.
    *
    * @param command the command to send to the radio without the escape character.
    * @param timeoutMillis the time in millis to wait for a response before returning.
    * @param esc escape character to add at the end of the command, defaults to
    * carriage return (CR).
    * @returns the standard Code enumeration.
    */
    virtual Code sendBasicCommand(const std::string& command, unsigned int timeoutMillis, char esc = CR);

    /** A static method for getting a string representation for the Registration
    * enumeration.
    *
    * @param code a Registration enumeration.
    * @returns the enumeration name as a string.
    */
    static std::string getRegistrationNames(Registration registration);

    /** A static method for getting a string representation for the Radio
    * enumeration.
    *
    * @param type a Radio enumeration.
    * @returns the enumeration name as a string.
    */
    static std::string getRadioNames(Radio radio);
    
    /** A method for changing the echo commands from radio.
    * @param state Echo mode is off (an argument of 1 turns echos off, anything else turns echo on)
    * @returns standard Code enumeration
    */
    virtual Code echo(bool state);
    
    /** This method can be used to trade socket functionality for performance.
    * Can disable checking socket closed messages from the data socket, and thus the socket
    * will only be visibly closed to the local side if the radio is explicitly checked, or
    * the socket is closed by the local side through the use of physical pin manipulation.
    *
    * Uses the Hayes escape sequence (1 second pause, "+++", 1 second pause) to exit the socket
    * connection to check if a received "NO CARRIER" string is from the radio indicating the socket
    * has been closed, or is merely part of the data stream. Should not occur very often, however, if 
    * data carrying the string "NO CARRIER" is going to be transmitted frequently, then the socket should
    * be set closeable and physical-socket-closing-means be used instead to reduce the large amount of
    * overhead switching from checking the validity of the "NO CARRIER" message being and indication of
    * the socket connection being closed.
    *
    * @param enabled set to true if you want the socket closeable, otherwise false. The default
    * is true.
    * @returns the standard AT Code enumeration.
    */
    virtual Code setSocketCloseable(bool enabled);
    
    /** Binds the socket to a specific port if able
    * @param port integer to bind the socket to.
    *   
    * @returns true if successfully bound port, false if bind failed.
    */
    virtual bool bind(unsigned int port);
    
    /** Checks if a socket is open.
    * @returns true if socket is open, false if socket is closed
    */
    virtual bool isOpen();
    
    /** Checks if there is data available from the socket.
    * @returns number of bytes of data available to read.
    */
    virtual unsigned int readable();
    
    /** Checks data to output on the socket
    * @returns number of bytes to be written to the socket.
    */
    virtual unsigned int writeable();
    
    /** Gets the device IP
    * @returns string containing the IP address
    */
    virtual std::string getDeviceIP();
    
    /** Sets the device IP
    * (Not implemented, IP address values are assigned by DHCP)
    * @returns true if the IP was set, false if IP address assignment failed.
    */
    virtual bool setDeviceIP(std::string address = "DHCP");
    
    /** Get the device IMEI or MEID (whichever is available)
    * @returns string containing the IMEI for GSM, the MEID for CDMA, or an empty string
    * if it failed to parse the number.
    */
    std::string getEquipmentIdentifier();

    /** Get radio type
    * @returns the radio type (MTSMC-H5, etc)
    */
    int getRadioType();

    /** Get string representation of radio type
    * @returns string containing the radio type (MTSMC-H5, etc)
    */
    std::string getRadioTypeString();

    /** Enables GPS.
    * @returns true if GPS is enabled, false if GPS is not supported.
    */
    virtual bool GPSenable();

    /** Disables GPS.
    * @returns true if GPS is disabled, false if GPS does not disable.
    */
    virtual bool GPSdisable();

    /** Checks if GPS is enabled.
    * @returns true if GPS is enabled, false if GPS is disabled.
    */
    virtual bool GPSenabled();
        
    /** Get GPS position.
    * @returns a structure containing the GPS data field information.
    */
    virtual gpsData GPSgetPosition();

    /** Check for GPS fix.
    * @returns true if there is a fix and false otherwise.
    */
    virtual bool GPSgotFix();

    bool socketOpened; //Specifies if a Socket is presently opened.
            
protected:
    MTSBufferedIO* io; //IO interface obect that the radio is accessed through.
    bool echoMode; //Specifies if the echo mode is currently enabled.

    bool gpsEnabled;    //true if GPS is enabled, else false.
        
    bool pppConnected; //Specifies if a PPP session is currently connected.
    std::string apn; //A string that holds the APN for the radio.

    Radio type; //The type of radio being used

    Mode socketMode; //The current socket Mode.
    bool socketCloseable; //Specifies is a Socket can be closed.
    unsigned int local_port; //Holds the local port for socket connections.
    std::string local_address; //Holds the local address for socket connections.
    unsigned int host_port; //Holds the remote port for socket connections.
    std::string host_address; //Holds the remote address for socket connections.

    DigitalIn* dcd; //Maps to the radio's dcd signal
    DigitalOut* dtr; //Maps to the radio's dtr signal
    DigitalOut* resetLine; //Maps to the radio's reset signal 
};

}

#endif /* CELLULAR_H */
