#ifndef CELLUTILS_H
#define CELLUTILS_H

#include "MTSBufferedIO.h"
#include "MTSLog.h"

using namespace mts;

//Special Payload Character Constants (ASCII Values)
const char ETX    = 0x03;   //Ends socket connection
const char DLE    = 0x10;   //Escapes ETX and DLE within Payload
const char CR     = 0x0D;   //Carriage Return
const char NL     = 0x0A;   //Newline
const char CTRL_Z = 0x1A;   //Control-Z

/// An enumeration for common responses.
enum Code {
    MTS_SUCCESS, MTS_ERROR, MTS_FAILURE, MTS_NO_RESPONSE
};

/** A static method for getting a string representation for the Code
* enumeration.
*
* @param code a Code enumeration.
* @returns the enumeration name as a string.
*/
static std::string getCodeNames(Code code)
{
    switch(code) {
        case MTS_SUCCESS:
            return "SUCCESS";
        case MTS_ERROR:
            return "ERROR";
        case MTS_NO_RESPONSE:
            return "NO_RESPONSE";
        case MTS_FAILURE:
            return "FAILURE";
        default:
            return "UNKNOWN ENUM";
    }
}

static string sendCommand(MTSBufferedIO* io, const std::string& command, unsigned int timeoutMillis, char esc = CR)
{
    if(io == NULL) {
        logError("MTSBufferedIO not set");
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

    int timer = 0;
    size_t previous = 0;
    char tmp[256];
    tmp[255] = 0;
    bool done = false;
    do {
        wait(0.1);
        timer += 100;

        previous = result.size();
        //Make a non-blocking read call by passing timeout of zero
        int size = io->read(tmp,255,0);    //1 less than allocated (timeout is instant)
        if(size > 0) {
            result.append(tmp, size);
        }
        done =  (result.size() == previous && previous > 0);
        if(timer >= timeoutMillis) {
            if (command != "AT" && command != "at") {
                logWarning("sendCommand [%s] timed out after %d milliseconds", command.c_str(), timeoutMillis);
            }
            done = true;
        }
    } while (!done);

    return result;
}

#endif
