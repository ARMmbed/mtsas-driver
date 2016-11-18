#ifndef MTSSERIAL_H
#define MTSSERIAL_H

#include "MTSBufferedIO.h"

namespace mts
{

/** This class derives from MTSBufferedIO and provides a buffered wrapper to the
* standard mbed Serial class. Since it depends only on the mbed Serial class for
* accessing serial data, this class is inherently portable accross different mbed
* platforms.
*/
class MTSSerial : public MTSBufferedIO
{
public:
    /** Creates a new MTSSerial object that can be used to talk to an mbed serial port
    * through internal SW buffers.
    *
    * @param TXD the transmit data pin on the desired mbed Serial interface.
    * @param RXD the receive data pin on the desired mbed Serial interface.
    * @param txBufferSize the size in bytes of the internal SW transmit buffer. The
    * default is 256 bytes.
    * @param rxBufferSize the size in bytes of the internal SW receive buffer. The
    * default is 256 bytes.
    */
    MTSSerial(PinName TXD, PinName RXD, int txBufferSize = 256, int rxBufferSize = 256);

    /** Destructs an MTSSerial object and frees all related resources, including
    * internal buffers.
    */
    ~MTSSerial();

    /** This method is used to the set the baud rate of the serial port.
    *
    * @param baudrate the baudrate in bps as an int. The default is 9600 bps.
    */
    void baud(int baudrate);

    /** This method sets the transmission format used by the serial port.
    *
    * @param bits the number of bits in a word (5-8; default = 8)
    * @param parity the parity used (SerialBase::None, SerialBase::Odd, SerialBase::Even,
    * SerialBase::Forced1, SerialBase::Forced0; default = SerialBase::None)
    * @param stop the number of stop bits (1 or 2; default = 1)
    */
    void format(int bits=8, SerialBase::Parity parity=mbed::SerialBase::None, int stop_bits=1);

protected:
    RawSerial serial; // Internal mbed Serial object

private:
    virtual void handleWrite(); // Method for handling data to be written
    virtual void handleRead(); // Method for handling data to be read
};

}

#endif /* MTSSERIAL_H */
