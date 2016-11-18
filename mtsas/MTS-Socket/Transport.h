#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "IPStack.h"

/** This class has been added to the standard mbed Socket library enabling people
* to use the Socket library interfaces for different transports. Use this class prior
* to instantiating any of the other classes in this folder to determine the underlying
* transport that will be used by them. It is important to know that the transport classes
* themsleves which derive from IPStack.h, must be properly initialized and connected before any
* of the Socket package classes can be used.
*/
class Transport
{
public:        
    /** This method allows you to set the transport to be used when creatin other
    * objects from the Socket folder like TCPSocketConnection and UDPSocket.
    *
    * @param type the type of underlying transport to be used as an IPStack object.
    */
    static void setTransport(IPStack* type);
    
    /** This method is used within the Socket class to get the appropraite transport
    * as an IPStack object.  In general you do not need to call this directly, but
    * simply use the other classes in this folder. 
    *
    * @returns a pointer to an object that implements IPStack.
    */
    static IPStack* getInstance();
    
private:
    static IPStack* transport; //Member variable that holds an custom transport type.
};

#endif /* TRANSPORT_H */
