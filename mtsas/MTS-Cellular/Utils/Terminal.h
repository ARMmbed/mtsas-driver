#ifndef TERMINAL_H
#define TERMINAL_H

#include "MTSBufferedIO.h"
#include "MTSSerial.h"

namespace mts
{

// A constant holding the exit message for the terminal program.
const std::string exitMsg = "emtech";

/** This class provides terminal style access to a serial interface on the
* processor. This is done by seamlessly "connecting" the data traffic between the
* mbed debug interface, usually accessed through the USB port, and the serial
* interface in question. Once started you can also exit this mode by sending the
* appropraite exit sequence through your terminal program. This sequence is: emtech
*/
class Terminal
{
public:
    /** This constructs a Terminal object that connects the standard
    * mbed USB serial inteface to the passed in serial interface. In
    * odrder for this class to function you must call the start method.
    *
    * @param io the serial interface to connect to the console as an
    * MTSBufferedIO object.
    */
    Terminal(MTSBufferedIO* io);
    
    /** Destructs an Terminal object and frees all created resources.
    */
    ~Terminal();
    
    /** This starts the terminal functionality and is a blocking call
    * until you send the exit sequence "emtech" through you terminal
    * program, at which point this method returns.
    */
    void start();
    
private:
    MTSBufferedIO* io; //The interface you want terminal access to
    MTSSerial* terminal; //The interface to the console or terminal
    int index; //The index you are at with the exit sequence
};

}

#endif /* TERMINAL_H */
