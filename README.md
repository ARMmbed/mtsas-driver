# mtsas-driver
Driver for the dragonfly cellular radio. 

## example usage
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
