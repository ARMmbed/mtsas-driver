#ifndef TESTMTSCIRCULARBUFFER_H
#define TESTMTSCIRCULARBUFFER_H

#include "MTSCircularBuffer.h"


/* unit tests for the circular buffer class */

using namespace mts;

class TestMTSCircularBuffer : public TestCollection
{
public:
    TestMTSCircularBuffer();
    virtual void run();
private:
    int capacity;
    MTSCircularBuffer buffer;
};

TestMTSCircularBuffer::TestMTSCircularBuffer() : TestCollection("MTSCircularBuffer"), capacity(0), buffer(5)
{
}

void TestMTSCircularBuffer::run()
{
    //Testing capacity method
    Test::start("capacity method");
    Test::assertTrue(buffer.capacity() == 5);
    Test::end();

    //Testing getSize method
    Test::start("size method");
    Test::assertTrue(buffer.size() == 0);
    buffer.write('A');
    Test::assertTrue(buffer.size() == 1);
    buffer.clear();
    Test::end();

    //Testing clear method
    Test::start("clear method");
    buffer.write("AT", 2);
    buffer.clear();
    Test::assertTrue(buffer.size() == 0);
    Test::end();
    
    //Test isEmpty method
    Test::start("isEmpty method");
    Test::assertTrue(buffer.isEmpty());
    buffer.write('A');
    Test::assertFalse(buffer.isEmpty());
    Test::end();
    
    //Test isFull method
    Test::start("isFull method");
    Test::assertFalse(buffer.isFull());
    buffer.write("12345", 5);
    Test::assertTrue(buffer.isFull());
    buffer.clear();
    Test::end();
}

#endif /* TESTMTSCIRCULARBUFFER_H */
