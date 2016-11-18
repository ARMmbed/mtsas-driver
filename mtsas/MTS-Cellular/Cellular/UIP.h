#ifndef UIP_H
#define UIP_H

#include <string>
#include <vector>

#include "MTSBufferedIO.h"
#include "Cellular.h"

namespace mts
{

/** This is a class for communicating with a Multi-Tech Systems SocketModem iCell. The
* SocketModem iCell is a family of carrier certified embedded cellular radio modules with
* a common hardware footprint and AT command set for built in IP-stack functionality.
* This class supports three main types of cellular radio interactions including:
* configuration and status AT command processing, SMS processing, and TCP Socket
* data connections. It should be noted that the radio can not process commands or
* SMS messages while having an open data connection at the same time. The concurrent
* capability may be added in a future release. This class also inherits from IPStack
* providing a common set of commands for communication devices that have an onboard
* IP Stack. It is also integrated with the standard mbed Sockets package and can therefore
* be used seamlessly with clients and services built on top of this interface already within
* the mbed library.
*
* All of the following examples use the Pin Names for the STMicro Nucleo F401RE board coupled with
* the SocketModem Shield Arduino compatible board. Please chage Pin Names accordingly to
* match your hardware configuration. It also assumes the use of RTS/CTS hardware handshaking
* using GPIOs. To disable this you will need to change settings on the radio module and
* and use the MTSSerial class instead of MTSSerialFlowControl. The default baud rate for the
* cellular radio is 115200 bps.
*
* Example code is found under Cellular.h
*/

class UIP : public Cellular
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
    * @returns a reference to the single Cellular object that has been created.
    */
    UIP(Radio type);

    /** Destructs a Cellular object and frees all related resources.
    */
    ~UIP();

    virtual bool init(MTSBufferedIO* io);

    // Cell connection based commands derived from CommInterface.h
    /** Initiates a PPP connection between the radio and the cell network */
    virtual bool connect();
    
    /** Disconnects the PPP connection between the radio and the cell network */
    virtual void disconnect();
    
    /** Checks if the radio has a PPP connection established with the cell network 
     * (Can reach the internet essentially)
     */
    virtual bool isConnected();
    
    /** Resets the radio, must first close active socket and PPP connections 
     * to do so
     */
    virtual void reset();

    // TCP and UDP Socket related commands
    // For behavior of the following methods refer to IPStack.h documentation
    virtual bool open(const std::string& address, unsigned int port, Mode mode);
    virtual bool close(bool shutdown);
    virtual int read(char* data, int max, int timeout = -1);
    virtual int write(const char* data, int length, int timeout = -1);
    virtual bool ping(const std::string& address = "8.8.8.8");
    
    /** A method for setting the APN 
    *
    * @param apn APN to be passed as a c-string
    * @returns the standard AT Code enumeration
    */
    virtual Code setApn(const std::string& apn);

};

}

#endif
