#include "mbed.h"
#include "MTSSerial.h"
#include "MTSLog.h"

using namespace mts;

MTSSerial::MTSSerial(PinName TXD, PinName RXD, int txBufferSize, int rxBufferSize)
    : MTSBufferedIO(txBufferSize, rxBufferSize)
    , serial(TXD,RXD)
{
    serial.attach(this, &MTSSerial::handleRead, Serial::RxIrq);
}

MTSSerial::~MTSSerial()
{
}

void MTSSerial::baud(int baudrate)
{
    serial.baud(baudrate);
}

void MTSSerial::format(int bits, SerialBase::Parity parity, int stop_bits)
{
    serial.format(bits, parity, stop_bits);
}

void MTSSerial::handleRead()
{
    char byte = serial.getc();
    if(rxBuffer.write(byte) != 1) {
        logError("Serial Rx Byte Dropped [%c][0x%02X]", byte, byte);
    }
}

void MTSSerial::handleWrite()
{
    while(txBuffer.size() != 0) {
        if (serial.writeable()) {
            char byte;
            if(txBuffer.read(byte) == 1) {
                serial.attach(NULL, Serial::RxIrq);
                serial.putc(byte);
                serial.attach(this, &MTSSerial::handleRead, Serial::RxIrq);
            }
        } else {
            return;
        }
    }
}


