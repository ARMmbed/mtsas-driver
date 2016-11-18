#ifndef IPSTACK_H
#define IPSTACK_H

#include <string>
#include "CommInterface.h"

/** This class is a pure virtual class that should be inherited from when implementing
* a communications device with an onboard IP stack.  Examples of this would be a Wi-Fi
* or Cellular radio. The inheriting class should map the device commands and functionality
* to the pure virtual methods provided here. There should also be at least one or more calls
* to setup the communication link specific paramters as an init method for example. This
* would do things like configure the APN in a cellular radio or set the ssid for a WiFi device,
* which cannot be accounted for in an abstract class like this one. Note that to provide physical
* connection management methods this class inherits from CommInterface.
*/
class IPStack : public CommInterface
{
public:
    /// An enumeration for selecting the Socket Mode of TCP or UDP.
    enum Mode {
        TCP, UDP
    };

    /** This method is used to set the local port for the UDP or TCP socket connection.
    * The connection can be made using the open method.
    *
    * @param port the local port of the socket as an int.
    */
    virtual bool bind(unsigned int port) = 0;

    /** This method is used to open a socket connection with the given parameters.
    *
    * @param address is the address you want to connect to in the form of xxx.xxx.xxx.xxx
    * or a URL. If using a URL make sure the device supports DNS and is properly configured
    * for that mode.
    * @param port the remote port you want to connect to.
    * @param mode an enum that specifies whether this socket connection is type TCP or UDP.
    * @returns true if the connection was successfully opened, otherwise false.
    */
    virtual bool open(const std::string& address, unsigned int port, Mode mode) = 0;

    /** This method is used to determine if a socket connection is currently open.
    *
    * @returns true if the socket is currently open, otherwise false.
    */
    virtual bool isOpen() = 0;

    /** This method is used to close a socket connection that is currently open.
    *
    * @returns true if successfully closed, otherwise returns false on an error.
    */
    virtual bool close(bool clearBuffer) = 0;

    /** This method is used to read data off of a socket, assuming a valid socket
    * connection is already open.
    *
    * @param data a pointer to the data buffer that will be filled with the read data.
    * @param max the maximum number of bytes to attempt to read, typically the same as
    * the size of the passed in data buffer.
    * @param timeout the amount of time in milliseconds to wait in trying to read the max
    * number of bytes. If set to -1 the call blocks until it receives the max number of bytes
    * or encounters and error.
    * @returns the number of bytes read and stored in the passed in data buffer. Returns
    * -1 if there was an error in reading.
    */
    virtual int read(char* data, int max, int timeout = -1) = 0;

    /** This method is used to write data to a socket, assuming a valid socket
    * connection is already open.
    *
    * @param data a pointer to the data buffer that will be written to the socket.
    * @param length the size of the data buffer to be written.
    * @param timeout the amount of time in milliseconds to wait in trying to write the entire
    * number of bytes. If set to -1 the call blocks until it writes all of the bytes or
    * encounters and error.
    * @returns the number of bytes written to the socket's write buffer. Returns
    * -1 if there was an error in writing.
    */
    virtual int write(const char* data, int length, int timeout = -1) = 0;

    /** This method is used to get the number of bytes available to read off the
    * socket.
    *
    * @returns the number of bytes available, 0 if there are no bytes to read.
    */
    virtual unsigned int readable() = 0;

    /** This method is used to get the space available to write bytes to the socket.
    *
    * @returns the number of bytes that can be written, 0 if unable to write.
    */
    virtual unsigned int writeable() = 0;

    /** This method is used test network connectivity by pinging a server.
    *
    * @param address the address of the server in format xxx.xxx.xxx.xxx. The
    * default 8.8.8.8 which is Google's DNS Server.
    * @returns true if the ping was successful, otherwise false.
    */
    virtual bool ping(const std::string& address = "8.8.8.8") = 0;

    /** This method is used to get the IP address of the device, which can be
    * set either statically or via DHCP after connecting to a network.
    *
    * @returns the devices IP address.
    */
    virtual std::string getDeviceIP() = 0;

    /** This method is used to set the IP address or puts the module in DHCP mode.
    *
    * @param address the IP address you want to use in the form of xxx.xxx.xxx.xxx or DHCP
    * if you want to use DHCP. The default is DHCP.
    * @returns true if successful, otherwise returns false.
    */
    virtual bool setDeviceIP(std::string address = "DHCP") = 0;
};

#endif /* IPSTACK_H */
