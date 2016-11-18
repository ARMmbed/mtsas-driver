#include "mbed.h"
#include "MTSSerialFlowControl.h"
#include "MTSLog.h"
#include "Utils.h"

using namespace mts;

MTSSerialFlowControl::MTSSerialFlowControl(PinName TXD, PinName RXD, PinName RTS, PinName CTS, int txBufSize, int rxBufSize)
    : MTSSerial(TXD, RXD, txBufSize, rxBufSize)
    , rxReadyFlag(false)
    , rts(RTS)
    , cts(CTS)
{
    notifyStartSending();

    // Calculate the high and low watermark values
    highThreshold = mts_max(rxBufSize - 10, rxBufSize * 0.85);
    lowThreshold = rxBufSize * 0.3;

    // Setup the low watermark callback on the internal receive buffer
    rxBuffer.attach(this, &MTSSerialFlowControl::notifyStartSending, lowThreshold, LESS);
}

MTSSerialFlowControl::~MTSSerialFlowControl()
{
}

//Override the rxClear function to make sure that flow control lines are set correctly.
void MTSSerialFlowControl::rxClear()
{
    MTSBufferedIO::rxClear();
    notifyStartSending();
}

void MTSSerialFlowControl::notifyStartSending()
{
    if(!rxReadyFlag) {
        rts.write(0);
        rxReadyFlag = true;
        //printf("RTS LOW: READY - RX[%d/%d]\r\n", rxBuffer.size(), rxBuffer.capacity());
    }
}

void MTSSerialFlowControl::notifyStopSending()
{
    if(rxReadyFlag) {
        rts.write(1);
        rxReadyFlag = false;
        //printf("RTS HIGH: NOT-READY - RX[%d/%d]\r\n", rxBuffer.size(), rxBuffer.capacity());
    }
}

void MTSSerialFlowControl::handleRead()
{
    char byte = serial.getc();
    if(rxBuffer.write(byte) != 1) {
        logError("Serial Rx Byte Dropped [%c][0x%02X]", byte, byte);
    }
    if (rxBuffer.size() >= highThreshold) {
        notifyStopSending();
    }
}

void MTSSerialFlowControl::handleWrite()
{
    while(txBuffer.size() != 0) {
        if (serial.writeable() && cts.read() == 0) {
            char byte;
            if(txBuffer.read(byte) == 1) {
                serial.attach(NULL, Serial::RxIrq);
                serial.putc(byte);
                serial.attach(this, &MTSSerialFlowControl::handleRead, Serial::RxIrq);
            }
        } else {
            return;
        }
    }
}

