#ifndef TESTHTTP_H
#define TESTHTTP_H

#include "mtsas.h"
#include <string>

using namespace mts;

class TestHTTP : public TestCollection
{
public:
    TestHTTP();
    virtual void run();
    
private:
    MTSSerialFlowControl* io;
    Cellular* radio;
    HTTPClient* http;
    char rbuf[1024];
    HTTPMap* send;
    HTTPText* receive;
};

TestHTTP::TestHTTP() : TestCollection("TestHTTP") {}

void TestHTTP::run() {
    
    string url;
    string base_url = "http://httpbin.org:80";
    string url_get = "/get";
    string url_put = "/put";
    string url_post = "/post";
    string url_del = "/delete";
    string url_stream = "/stream/20";
    
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
    
    http = new HTTPClient();
    send = new HTTPMap();
    receive = new HTTPText(rbuf);
    
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
    Test::end();
    
    url = base_url + url_get;
    Test::start("HTTP GET");
    Test::assertTrue(http->get(url.c_str(), receive, 10000) == HTTP_OK);
    Test::end();
    
    url = base_url + url_put;
    Test::start("HTTP PUT");
    send->put("testing", "put");
    Test::assertTrue(http->put(url.c_str(), *send, receive, 10000) == HTTP_OK);
    Test::end();
    
    url = base_url + url_post;
    Test::start("HTTP POST");
    send->put("testing", "put");
    Test::assertTrue(http->post(url.c_str(), *send, receive, 10000) == HTTP_OK);
    Test::end();
    
    url = base_url + url_del;
    Test::start("HTTP DELETE");
    Test::assertTrue(http->del(url.c_str(), receive, 10000) == HTTP_OK);
    Test::end();
    
    url = base_url + url_stream;
    char bigbuf[8 * 1024];
    HTTPText big(bigbuf);
    Test::start("HTTP big GET");
    Test::assertTrue(http->get(url.c_str(), &big, 5000) == HTTP_OK);
    Test::end();
    
    radio->disconnect();
}

#endif