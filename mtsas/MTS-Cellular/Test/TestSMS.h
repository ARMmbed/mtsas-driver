#ifndef TESTSMS_H
#define TESTSMS_H

#include "mtsas.h"
#include <string>
#include <vector>

using namespace mts;

class TestSMS : public TestCollection
{
public:
    TestSMS();
    virtual void run();
    
private:
    MTSSerialFlowControl* io;
    Cellular* radio;
};


TestSMS::TestSMS() : TestCollection("TestSMS") {}

void TestSMS::run() {
    
    string number;
    string response;
    vector<string> parts;
    vector<Cellular::Sms> rmessages;
    vector<string> smessages;
    smessages.push_back("testing SMS 1");
    smessages.push_back("testing SMS 2");
    smessages.push_back("This is a longer SMS message.  It is the third and final message that will be sent.");
    
    MTSLog::setLogLevel(MTSLog::TRACE_LEVEL);
    
    Test::start("Setup");
    io = new MTSSerialFlowControl(D8, D2, D3, D6);
    io->baud(115200);
    radio = CellularFactory::create(io);
    if (! radio) {
        logError("radio is NULL");
        Test::assertTrue(false);
    }
    radio->configureSignals(D4, D7, RESET);
    
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
    
    //Wait until the SIM card is ready
    while (radio->sendBasicCommand("AT+CMGD=1,4", 3000) != MTS_SUCCESS);
    
    Test::assertTrue(radio->deleteAllReceivedSms() == MTS_SUCCESS);
    Test::assertTrue(radio->getReceivedSms().size() == 0);
    Test::end();
    
    Test::start("Send SMS 1");
    logInfo("finding my phone number");
    while (true) {
        response = radio->sendCommand("AT+CNUM", 1000);
        if (response.find("ERROR") == string::npos && response.find("error") == string::npos) {
            break;
        }
        wait(1);
    }
    
    //Read phone number from radio
    if (response.find("+CNUM:") != string::npos) {
        parts = Text::split(response, ",");
        number = parts[1];
        size_t fquote = number.find("\"");
        size_t bquote = number.rfind("\"");
        number = number.substr(fquote + 1, bquote - 1);
        logInfo("My phone number: [%s]", number.c_str());
    } else {
        Test::assertTrue(false);
    }
    
    Test::assertTrue(radio->sendSMS(number, smessages[0]) == MTS_SUCCESS);
    Test::end();
    Test::start("Send SMS 2");
    Test::assertTrue(radio->sendSMS(number, smessages[1]) == MTS_SUCCESS);
    Test::end();
    
    wait(30);
    
    Test::start("Receive SMS");
    rmessages = radio->getReceivedSms();
    Test::assertTrue(rmessages.size() == 2);
    for (int i = 0; i < rmessages.size(); i++) {
        Test::assertTrue(rmessages[i].message == smessages[i]);
    }
    Test::end();
    
    Test::start("Send another SMS");
    Test::assertTrue(radio->sendSMS(number, smessages[2]) == MTS_SUCCESS);
    Test::end();
    
    wait(30);
    
    /* the SMS that we haven't "read" yet should not get deleted */
    Test::start("Delete Read SMS Messages");
    Test::assertTrue(radio->deleteOnlyReceivedReadSms() == MTS_SUCCESS);
    Test::assertTrue(radio->getReceivedSms().size() == 1);
    Test::end();
    
    Test::start("Receive another SMS");
    rmessages = radio->getReceivedSms();
    Test::assertTrue(rmessages.size() == 1);
    Test::assertTrue(rmessages[0].message == smessages[2]);
    Test::end();
    
    Test::start("Delete All SMS Messages");
    Test::assertTrue(radio->deleteAllReceivedSms() == MTS_SUCCESS);
    Test::assertTrue(radio->getReceivedSms().size() == 0);
    Test::end();
}

#endif