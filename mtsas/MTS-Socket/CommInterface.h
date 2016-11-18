#ifndef COMMINTERFACE_H
#define COMMINTERFACE_H

/** This pure virtual class for communications link of interface. This class
* should be derived from when creating a class to manage the underlying connection
* of a new link type.
*/
class CommInterface
{
public:
    /** This method is used to establish a connection on a communications link. Required
    * configurations and settings should be done in other calls or an init function.
    *
    * @returns true if the connection was successfully established, otherwise false.
    */
    virtual bool connect() = 0;

    /** This method is used to disconnect a communications like. This includes
    * any cleanup required before another connection can be made.
    */
    virtual void disconnect() = 0;

    /** This method is used to check if the link is currently connected.
    *
    * @returns true if currently connected, otherwise false.
    */
    virtual bool isConnected() = 0;
    
    /** This method is used to reset the device that provides the communications
    * capability. Note that this call should block until the commincations device
    * is ready for use.
    */
    virtual void reset() = 0;
};

#endif /* COMMINTERFACE_H */
