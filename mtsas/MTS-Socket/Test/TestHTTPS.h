#ifndef TESTHTTPS_H
#define TESTHTTPS_H

#include "mtsas.h"
#include <string>
#include "httpbinCert.h"

using namespace mts;

class TestHTTPS : public TestCollection
{
public:
    TestHTTPS();
    virtual void run();
    
private:
    MTSSerialFlowControl* io;
    Cellular* radio;
    HTTPClient* http;
    char rbuf[1024];
    HTTPMap* send;
    HTTPText* receive;
};

TestHTTPS::TestHTTPS() : TestCollection("TestHTTPS") {}

void TestHTTPS::run() {
    
    
    string url;
    string secure_url = "https://httpbin.org:443";
    string url_get = "/get";
    string url_put = "/put";
    string url_post = "/post";
    string url_del = "/delete";
    string url_stream = "/stream/20";
    string url_auth = "/basic-auth";
    
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
    
    if (http->addRootCACertificate(CERTIFICATE) != HTTP_OK) {
        Test::assertTrue(false);
    }
    http->setPeerVerification(VERIFY_PEER);
    
    Test::end();
    
    url = secure_url + url_get;
    Test::start("HTTPS GET");
    Test::assertTrue(http->get(url.c_str(), receive, 10000) == HTTP_OK);
    Test::end();
    
    url = secure_url + url_put;
    Test::start("HTTPS PUT");
    send->put("testing", "put");
    Test::assertTrue(http->put(url.c_str(), *send, receive, 10000) == HTTP_OK);
    send->clear();
    Test::end();
    
    url = secure_url + url_post;
    Test::start("HTTPS POST");
    send->put("testing", "put");
    Test::assertTrue(http->post(url.c_str(), *send, receive, 10000) == HTTP_OK);
    send->clear();
    Test::end();
    
    url = secure_url + url_del;
    Test::start("HTTPS DELETE");
    Test::assertTrue(http->del(url.c_str(), receive, 10000) == HTTP_OK);
    Test::end();
    
    url = secure_url + url_stream;
    char bigbuf[8 * 1024];
    HTTPText big(bigbuf);
    Test::start("HTTPS big GET");
    Test::assertTrue(http->get(url.c_str(), &big, 10000) == HTTP_OK);
    Test::end();
    
    string url_usr_pw = "/Bob/123";
    url = secure_url + url_auth + url_usr_pw;
    const char user[] = "Bob",pw[] = "123";
    Test::start("HTTPS AUTH GET");
    Test::assertTrue(http->basicAuth(user,pw) == HTTP_OK);
    Test::assertTrue(http->get(url.c_str(), receive, 30000) == HTTP_OK);
    Test::assertTrue(http->basicAuth(NULL,NULL) == HTTP_OK);
    Test::end();
    
    radio->disconnect();
}

#endif