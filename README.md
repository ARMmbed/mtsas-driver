# mtsas-driver
Driver for the dragonfly cellular radio. 

## sockets example 
```C++
#include "mbed.h"
#include "TCPSocket.h"
#include "MTSASInterface.h"

MTSASInterface cell(RADIO_TX, RADIO_RX, true);

// Socket demo
void http_demo(NetworkInterface *net) {
    TCPSocket socket;

    // Show the network address
    const char *ip = net->get_ip_address();
    printf("IP address is: %s\n", ip ? ip : "No IP");

    // Open a socket on the network interface, and create a TCP connection to mbed.org
    socket.open(net);
    socket.connect("developer.mbed.org", 80);

    // Send a simple http request
    char sbuffer[] = "GET / HTTP/1.1\r\nHost: developer.mbed.org\r\n\r\n";
    int scount = socket.send(sbuffer, sizeof sbuffer);
    printf("sent %d [%.*s]\r\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);

    // Recieve a simple http response and print out the response line
    char rbuffer[64];
    int rcount = socket.recv(rbuffer, sizeof rbuffer);
    printf("recv %d [%.*s]\r\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);

    // Close the socket to return its memory and bring down the network interface
    socket.close();
}

int main() {
    // Brings up the dragonfly
    printf("cell socket example\r\n");
    // An APN is required for GSM radios.
    static const char apn[] = "wap.cingular";
    cell.connect(apn);
    printf("Connected\r\n");
    
    // Invoke the demo
    http_demo(&cell);
    
    //Get gps location
    printf("Getting GPS Coords\r\n");
    gps_data loc = cell.get_gps_location();
    printf("UTC: %s lat:%f lon: %f\r\n", loc.UTC, loc.latitude, loc.longitude);
    
    // Brings down the dragonfly
    cell.disconnect();
    printf("Done\n");
}
```


## sms text example 
```C++
#include "mbed.h"
#include "MTSASInterface.h"

MTSASInterface cell(RADIO_TX, RADIO_RX);

void print_sms(char* msg){
    printf("msg: %s\r\n", msg);
}

int main() {
    // Brings up the dragonfly
    printf("cell texting example\r\n");
    // An APN is required for GSM radios.
    static const char apn[] = "wap.cingular";
    printf("connecting to network...\r\n");
    cell.connect(apn);
    printf("connected!\r\n");

    //Handle incoming messages with print_sms
    cell.sms_attach(&print_sms);
    printf("listening for texts\r\n");

    while(true){}

    cell.disconnect();
    printf("Done\n");
}
```
