#include "mbed.h"
#include "MTSCircularBuffer.h"

using namespace mts;

MTSCircularBuffer::MTSCircularBuffer(int bufferSize) : bufferSize(bufferSize), readIndex(0), writeIndex(0), bytes(0), _threshold(-1), _op(GREATER)
{
    buffer = new char[bufferSize];
}

MTSCircularBuffer::~MTSCircularBuffer()
{
    delete[] buffer;
}

int MTSCircularBuffer::read(char* data, int length)
{
    int i = 0;
    while ((i < length) && (bytes > 0)) {
        if (readIndex == bufferSize) {
            readIndex = 0;
        }
        __disable_irq();
        data[i++] = buffer[readIndex++];
        bytes--;
        __enable_irq();
        checkThreshold();
    }
    return i;
}

int MTSCircularBuffer::read(char& data)
{
    if (bytes == 0) {
        return 0;
    }
    if (readIndex == bufferSize) {
        readIndex = 0;
    }
    __disable_irq();
    data = buffer[readIndex++];
    bytes--;
    __enable_irq();
    checkThreshold();
    return 1;
}

int MTSCircularBuffer::write(const char* data, int length)
{
    int i = 0;
    while((i < length) && (bytes < bufferSize)) {
        if(writeIndex == bufferSize) {
            writeIndex = 0;
        }
        __disable_irq();
        buffer[writeIndex++] = data[i++];
        bytes++;
        __enable_irq();
        checkThreshold();
    }
    return i;
}

int MTSCircularBuffer::write(char data)
{
    if (bytes == bufferSize) {
        return 0;
    }
    if(writeIndex == bufferSize) {
        writeIndex = 0;
    }
    __disable_irq();
    buffer[writeIndex++] = data;
    bytes++;
    __enable_irq();
    checkThreshold();
    return 1;
}

int MTSCircularBuffer::capacity()
{
    return bufferSize;
}

int MTSCircularBuffer::remaining()
{
    return bufferSize - bytes;
}

int MTSCircularBuffer::size()
{
    return bytes;
}

bool MTSCircularBuffer::isFull()
{
    if (bytes == bufferSize) {
        return true;
    } else {
        return false;
    }
}

bool MTSCircularBuffer::isEmpty()
{
    if (bytes == 0) {
        return true;
    } else {
        return false;
    }
}

void MTSCircularBuffer::clear()
{
    writeIndex = readIndex = bytes = 0;
}

void MTSCircularBuffer::checkThreshold()
{
    if (_threshold == -1) {
        return;
    }
    switch (_op) {
        case GREATER:
            if (bytes > _threshold) {
                notify.call();
            }
            break;
        case LESS:
            if (bytes < _threshold) {
                notify.call();
            }
            break;
        case GREATER_EQUAL:
            if (bytes >= _threshold) {
                notify.call();
            }
            break;
        case LESS_EQUAL:
            if (bytes <= _threshold) {
                notify.call();
            }
            break;
        case EQUAL:
            if (bytes == _threshold) {
                notify.call();
            }
            break;
    }
}

