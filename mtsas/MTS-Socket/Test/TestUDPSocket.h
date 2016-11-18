#ifndef TESTUDPSOCKET_H
#define TESTUDPSOCKET_H

#include "mtsas.h"

using namespace mts;

class TestUDPSocket : public TestCollection
{
public:
    TestUDPSocket();
    virtual void run();

private:
    MTSSerialFlowControl* io;
    Cellular* radio;
    UDPSocket* sock;
};

//Must set test server and test port before running socket test
const char UDP_TEST_SERVER[] = "";
const int UDP_TEST_PORT = -1;

//Test pattern can be changed to any alphanumeric pattern wanted
const char TEST_PATTERN[] = "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}|\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}/\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}-\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}\\\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}|\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}/\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}-\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}\\\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}*\r\n";

TestUDPSocket::TestUDPSocket() : TestCollection("TestUDPSocket") {}

void TestUDPSocket::run() {
    
    Endpoint remote;
    remote.set_address(UDP_TEST_SERVER, UDP_TEST_PORT);
    
    MTSLog::setLogLevel(MTSLog::TRACE_LEVEL);
    Test::start("Setup");
    io = new MTSSerialFlowControl(D8, D2, D3, D6);
    io->baud(115200);
    radio = CellularFactory::create(io);
    if (! radio) {
        Test::assertTrue(false);
    }
    radio->configureSignals(D4, D7, RESET);
    Transport::setTransport(radio);

    for (int i = 0; i < 10; i++) {
        if (i >= 10) {
            Test::assertTrue(false);
        }
        if (radio->setApn(APN) == MTS_SUCCESS) {
            break;
        } else {
            wait(1);
        }
    }
    
    for (int i = 0; i < 3; i++) {
        if (i >= 3) {
            Test::assertTrue(false);
        }
        if (radio->connect()) {
            break;
        } else {
            wait(1);
        }
    }

    for (int i = 0; i < 5; i++) {
        if (i >= 5) {
            Test::assertTrue(false);
        }
        if (radio->ping()) {
            break;
        } else {
            wait(1);
        }
    }

    sock = new UDPSocket();
    sock->set_blocking(false, 20000);
    Test::end();
    
    /** set up an UDP echo server using netcat
     * nc -e /bin/cat -l -u -p TEST_PORT
     * this should echo back anything you send to it.
     * Note: Listen port might close after each UDP packet
     * if testing UIP-type radios.
     */
    
    int bufsize = 1024;
    char buf[bufsize];
    int size = 0;
     
    for (int i = 0; i < 10; i++) {
        Test::start("Test UDP");
        logInfo("sending to remote");
        size = sock->sendTo(remote, (char*) TEST_PATTERN, sizeof(TEST_PATTERN));
        if (size != sizeof(TEST_PATTERN)) {
            logError("failed to send - only sent %d bytes", size);
            Test::assertTrue(false);
        }
        logInfo("receiving from remote");
        size = sock->receiveFrom(remote, buf, bufsize);
        if (size != sizeof(TEST_PATTERN)) {
            logError("failed to receive - only received %d bytes", size);
            Test::assertTrue(false);
        } else {
            for (int j = 0; j < sizeof(TEST_PATTERN); j++) {
                if (buf[j] != TEST_PATTERN[j]) {
                    logError("patterns don't match at %d, pattern [%#02X] received [%#02X]", j, TEST_PATTERN[j], buf[j]);
                    Test::assertTrue(false);
                }
            }
        }
        Test::end();
    }
    sock->close();
    radio->disconnect();
}

#endif