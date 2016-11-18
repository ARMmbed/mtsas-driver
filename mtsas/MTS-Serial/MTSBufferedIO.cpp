#include "mbed.h"
#include "MTSBufferedIO.h"
#include "Utils.h"

using namespace mts;

MTSBufferedIO::MTSBufferedIO(int txBufferSize, int rxBufferSize)
: txBuffer(txBufferSize)
, rxBuffer(rxBufferSize)
{
}

MTSBufferedIO::~MTSBufferedIO()
{
}

int MTSBufferedIO::writef(const char* format, ...) {
    char buff[256];

    va_list ap;
    va_start(ap, format);
    int len = vsnprintf(buff, 256, format, ap);
    while (!writeable())
        ;
    write(buff, len);
    va_end(ap);
    return len;
}

int MTSBufferedIO::write(const char* data, int length, unsigned int timeoutMillis) 
{
    //Writes until empty or timeout is reached (different implementation planned once tx isr is working)
    int bytesWritten = 0;
    Timer tmr;
    tmr.start();
    length = mts_max(0,length);
    do {
        int bytesWrittenSwBuffer = txBuffer.write(&data[bytesWritten], length - bytesWritten);
        if(bytesWrittenSwBuffer > 0) {
            handleWrite();
            int bytesRemainingSwBuffer = txBuffer.size();
            txBuffer.clear();
            bytesWritten += (bytesWrittenSwBuffer - bytesRemainingSwBuffer);
        }
    } while(tmr.read_ms() <= timeoutMillis && bytesWritten < length);
    return bytesWritten;
}

int MTSBufferedIO::write(const char* data, int length)
{   
    //Blocks until all bytes are written (different implementation planned once tx isr is working)
    int bytesWritten = 0;
    length = mts_max(0,length);
    do {
        int bytesWrittenSwBuffer = txBuffer.write(&data[bytesWritten], length - bytesWritten);
        handleWrite();
        int bytesRemainingSwBuffer = txBuffer.size();
        txBuffer.clear();
        bytesWritten += bytesWrittenSwBuffer - bytesRemainingSwBuffer;
    } while(bytesWritten < length);
    return length;
}

int MTSBufferedIO::write(char data, unsigned int timeoutMillis) 
{
    return write(&data, 1, timeoutMillis);
}

int MTSBufferedIO::write(char data)
{
    return write(&data, 1);
}

int MTSBufferedIO::writeable() {
    return txBuffer.remaining();   
}

int MTSBufferedIO::read(char* data, int length, unsigned int timeoutMillis) 
{
    int bytesRead = 0;
    Timer tmr;
    tmr.start();
    length = mts_max(0,length);
    do {
        bytesRead += rxBuffer.read(&data[bytesRead], length - bytesRead);
    } while(tmr.read_ms() <= timeoutMillis && bytesRead < length);
    return bytesRead;
}

int MTSBufferedIO::read(char* data, int length)
{
    int bytesRead = 0;
    length = mts_max(0,length);
    while(bytesRead < length) {
        bytesRead += rxBuffer.read(&data[bytesRead], length - bytesRead);
    }
    return length;
}

int MTSBufferedIO::read(char& data, unsigned int timeoutMillis) 
{
    return read(&data, 1, timeoutMillis);
}

int MTSBufferedIO::read(char& data)
{
    return rxBuffer.read(&data, 1);
}

int MTSBufferedIO::readable() {
    return rxBuffer.size();   
}

bool MTSBufferedIO::txEmpty()
{
    return txBuffer.isEmpty();
}

bool MTSBufferedIO::rxEmpty()
{
    return rxBuffer.isEmpty();
}

bool MTSBufferedIO::txFull()
{
    return txBuffer.isFull();
}

bool MTSBufferedIO::rxFull()
{
    return rxBuffer.isFull();
}

void MTSBufferedIO::txClear()
{
    txBuffer.clear();
}

void MTSBufferedIO::rxClear()
{
    rxBuffer.clear();
}
