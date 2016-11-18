#ifndef DOTESTS_H
#define DOTESTS_H

/*Input APN for cellular tests that use APN*/
static char* APN = "";

#include "TestRunner.h"
#include <vector>

#include "TestMTSText.h"
#include "TestMTSLog.h"
#include "TestMTSCircularBuffer.h"
#include "TestHTTP.h"
#include "TestHTTPS.h"
#include "TestTCPSocketConnection.h"
#include "TestUDPSocket.h"
#include "TestSMS.h"

using namespace mts;

static void doTests() {
    std::vector<TestCollection*> collections;
    
    /* Note: Socket reception timeout values can be changed within the specific test header files if the radio is
        moving on too soon before receiving data */
    collections.push_back(new TestMTSText);
    collections.push_back(new TestMTSLog);
    collections.push_back(new TestMTSCircularBuffer);
    collections.push_back(new TestHTTP);
    collections.push_back(new TestHTTPS);
    collections.push_back(new TestTCPSocketConnection);
    collections.push_back(new TestUDPSocket);
    collections.push_back(new TestSMS);

    for (std::vector<TestCollection*>::iterator it = collections.begin(); it != collections.end(); it++) {
        TestRunner::addCollection(*it);
    }

    TestRunner::runTests(true, true, true, true);
}

#endif