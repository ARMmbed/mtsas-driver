#ifndef EASYIP_H
#define EASYIP_H

#include <string>
#include <vector>

#include "MTSBufferedIO.h"
#include "Cellular.h"

namespace mts
{
    /** This class implements the same interface used for UIP version radios on an Easy IP radio
    * using the Hayes AT command set.
    * (See the UIP class and documentation, "UIP.h")
    * This class supports four main types of cellular radio interactions including:
    * configuration and status AT-command processing, SMS processing, TCP Socket data connections,
    * and UDP data connections. It should be noted that the radio can not process commands or
    * SMS messages while having an open data connection at the same time. The concurrent
    * capability may be added in a future release. This class also inherits from IPStack
    * providing a common set of commands for communication devices that have an onboard
    * IP Stack. It is also integrated with the standard mbed Sockets package and can therefore
    * be used seamlessly with clients and services built on top of this interface already within
    * the mbed library.
    * The default baud rate for the cellular radio is 115200 bps.
    *
    * Example code is found under Cellular.h
    */
class EasyIP : public Cellular
{
public:
    /** This static function is used to create or get a reference to a
    * Cellular object. Cellular uses the singleton pattern, which means
    * that you can only have one existing at a time. The first time you
    * call getInstance this method creates a new uninitialized Cellular
    * object and returns it. All future calls to this method will return
    * a reference to the instance created during the first call. Note that
    * you must call init on the returned instance before mnaking any other
    * calls. If using this class's bindings to any of the Socket package
    * classes like TCPSocketConnection, you must call this method and the
    * init method on the returned object first, before even creating the
    * other objects.
    *
    * @returns a reference to the single Cellular obect that has been created.
    */
    EasyIP(Radio type);
    
    /** Destructs a Cellular object and frees all related resources.
    */
    ~EasyIP();
    
    /** Initializes the MTS IO buffer
    */
    virtual bool init(MTSBufferedIO* io);
    
    /** PPP connect command.
    * Connects the radio to the cellular network.
    *
    * @returns true if PPP connection to the network succeeded,
    * false if the PPP connection failed.
    */
    virtual bool connect();
    
    /** PPP disconnect command.
    * Disconnects from the PPP network, and will also close active socket
    * connection if open. 
    */
    virtual void disconnect();
    
    /** Checks if the radio is connected to the cell network.
    * Checks antenna signal, cell tower registration, and context activation
    * before finally pinging (4 pings, 32 bytes each) to confirm PPP connection
    * to network. Will return true if there is an open socket connection as well.
    *
    * @returns true if there is a PPP connection to the cell network, false
    * if there is no PPP connection to the cell network.
    */
    virtual bool isConnected();
    
    /** Resets the radio/modem.
    * Disconnects all active PPP and socket connections to do so.
    */
    virtual void reset();

    // TCP and UDP Socket related commands
    // For behavior of the following methods refer to IPStack.h documentation
    
    virtual bool open(const std::string& address, unsigned int port, Mode mode);
    virtual bool close(bool shutdown);
    virtual int read(char* data, int max, int timeout = -1);    
    virtual int write(const char* data, int length, int timeout = -1);
    
    /** Pings specified DNS or IP address
     * Google DNS server used as default ping address
     * @returns true if ping received alive response else false
     */
    virtual bool ping(const std::string& address = "8.8.8.8"); 
    
    /** Sets the APN
    * 
    * @param apn c-string of the APN to use
    *
    * @returns MTS_SUCCESS if the APN was set, or is not needed, else it
    * returns the result of the AT command sent to the radio to set the APN
    */
    virtual Code setApn(const std::string& apn);
    
    /** Enables GPS.
    * @returns true if GPS gets or is enabled, false if GPS is not supported.
    */
    virtual bool GPSenable();

    /** Disables GPS.
    * @returns true if GPS gets or is disabled, false if GPS failed to disable.
    */
    virtual bool GPSdisable();

    /** Checks if GPS is enabled.
    * @returns true if GPS is enabled, false if GPS is disabled.
    */
    virtual bool GPSenabled();
        
    /** Get GPS position.
    * @returns a string containing the GPS position.
    */
    virtual Cellular::gpsData GPSgetPosition();

    /** Check for GPS fix.
    * @returns true if there is a fix and false otherwise.
    */
    virtual bool GPSgotFix();
        
private:
    /** Function that sends +++ to the radio to exit data mode
    * returns true if it successfully exits from online mode, else
    * it returns false. Used due to the fact that AT commands
    * cannot be sent while in data mode.
    *
    * @returns true if the radio dropped from data mode to commande mode
    * or is already in command mode (socket is still open in the background),
    * and false if the radio failed to switch to command mode.
    */
    virtual bool sendEscapeCommand(); 
    
    /** Switches to command mode, queries the status of the socket connection,
    * and then returns back to the active socket connection (if still open)
    *
    * @returns true if a socket is currently open, otherwise it returns false
    */
    virtual bool socketCheck();
};

}

#endif /* EASYIP_H */