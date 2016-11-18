#ifndef TESTTCPSOCKETCONNECTION_H
#define TESTTCPSOCKETCONNECTION_H

#include "mtsas.h"
#include <string>

using namespace mts;

class TestTCPSocketConnection : public TestCollection
{
public:
    TestTCPSocketConnection();
    virtual void run();

private:
    bool runIteration();
    MTSSerialFlowControl* io;
    Cellular* radio;
    TCPSocketConnection* sock;
};

const char PATTERN_LINE1[] = "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}|";
const char PATTERN[] = "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}|\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}/\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}-\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}\\\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}|\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}/\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}-\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}\\\r\n"
                       "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}*\r\n";

const char MENU_LINE1[] = "send ascii pattern until keypress";
const char MENU[] = "1       send ascii pattern until keypress"
                    "2       send ascii pattern (numbered)"
                    "3       send pattern and close socket"
                    "4       send [ETX] and wait for keypress"
                    "5       send [DLE] and wait for keypress"
                    "6       send all hex values (00-FF)"
                    "q       quit"
                    ">:";

const char TCP_TEST_SERVER[] = "204.26.122.5";
const int TCP_TEST_PORT = 7000;

TestTCPSocketConnection::TestTCPSocketConnection() : TestCollection("TestTCPSocketConnection") {}

void TestTCPSocketConnection::run()
{
    
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

    sock = new TCPSocketConnection();
    sock->set_blocking(false, 2);
    Test::end();

    for (int i = 0; i < 10; i++) {
        Test::start("Test TCP");
        Test::assertTrue(runIteration());
        Test::end();
    }
    
    radio->disconnect();
}

bool TestTCPSocketConnection::runIteration()
{
    Timer tmr;
    int bytesRead = 0;
    const int readSize = 1024;
    char buffer[readSize] = {0};
    string result;

    for (int i = 0; i < 5; i++) {
        if (i >= 5) {
            return false;
        }
        if (! sock->connect(TCP_TEST_SERVER, TCP_TEST_PORT)) {
            break;
        } else {
            wait(1);
        }
    }

    logInfo("Receiving Menu");
    tmr.reset();
    tmr.start();
    do {
        bytesRead = sock->receive(buffer, readSize);
        if (bytesRead > 0) {
            result.append(buffer, bytesRead);
        }
        logInfo("Total Bytes Read: %d", result.size());
        if(result.find(MENU) != std::string::npos) {
            break;
        }
    } while(tmr.read() <= 40);

    logInfo("Received: [%d] [%s]", result.size(), result.c_str());

    size_t pos = result.find(MENU_LINE1);
    if(pos != string::npos) {
        logInfo("Found Menu 1st Line");
    } else {
        logError("Failed To Find Menu 1st Line");
        sock->close();
        return false;
    }

    result.clear();

    logInfo("Writing To Socket: 2");
    if(sock->send("2\r\n", 3) == 3) {
        logInfo("Successfully Wrote '2'");
    } else {
        logError("Failed To Write '2'");
        sock->close();
        return false;
    }
    logInfo("Expecting 'how many ? >:'");
    tmr.reset();
    tmr.start();
    do {
        bytesRead = sock->receive(buffer, readSize);
        if (bytesRead > 0) {
            result.append(buffer, bytesRead);
        }
        logInfo("Total Bytes Read: %d", result.size());
        if(result.find("how many") != std::string::npos) {
            break;
        }
    } while(tmr.read() <= 40);

    logInfo("Received: [%d] [%s]", result.size(), result.c_str());

    if(result.find("how many") != std::string::npos) {
        logInfo("Successfully Found 'how many'");
        logInfo("Writing To Socket: 2");
        if(sock->send("2\r\n", 3) == 3) {
            logInfo("Successfully wrote '2'");
        } else {
            logError("Failed to write '2'");
            sock->close();
            return false;
        }
    } else {
        logError("didn't receive 'how many'");
        sock->close();
        return false;
    }

    result.clear();

    logInfo("Receiving Data");
    tmr.reset();
    tmr.start();
    do {
        bytesRead = sock->receive(buffer, readSize);
        if (bytesRead > 0) {
            result.append(buffer, bytesRead);
        }
        logInfo("Total Bytes Read: %d", result.size());
        if(result.size() >= 1645) {
            break;
        }
    } while(tmr.read() <= 40);

    logInfo("Received Data: [%d] [%s]", result.size(), result.c_str());

    pos = result.find(PATTERN_LINE1);
    if(pos != string::npos) {
        int patternSize = sizeof(PATTERN) - 1;
        const char* ptr = &result.data()[pos];
        bool match = true;
        for(int i = 0; i < patternSize; i++) {
            if(PATTERN[i] != ptr[i]) {
                logError("1st Pattern Doesn't Match At [%d]", i);
                logError("Pattern [%02X]  Buffer [%02X]", PATTERN[i], ptr[i]);
                match = false;
                break;
            }
        }
        if(match) {
            logInfo("Found 1st Pattern");
        } else {
            logError("Failed To Find 1st Pattern");
            sock->close();
            return false;
        }

        pos = result.find(PATTERN_LINE1, pos + patternSize);
        if(pos != std::string::npos) {
            ptr = &result.data()[pos];
            match = true;
            for(int i = 0; i < patternSize; i++) {
                if(PATTERN[i] != ptr[i]) {
                    logError("2nd Pattern Doesn't Match At [%d]", i);
                    logError("Pattern [%02X]  Buffer [%02X]", PATTERN[i], ptr[i]);
                    match = false;
                    break;
                }
            }
            if(match) {
                logInfo("Found 2nd Pattern");
            } else {
                logError("Failed To Find 2nd Pattern");
                sock->close();
                return false;
            }
        }
    } else {
        logError("Failed To Find Pattern 1st Line");
        sock->close();
        return false;
    }

    result.clear();
    sock->close();
    radio->disconnect();
    return true;
}

#endif