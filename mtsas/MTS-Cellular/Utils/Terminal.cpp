#include "mbed.h"
#include "Terminal.h"
#include "MTSLog.h"

using namespace mts;

Terminal::Terminal(MTSBufferedIO* io) : io(io), index(0)
{
    terminal = new MTSSerial(USBTX, USBRX);
}

Terminal::~Terminal()
{
    delete terminal; 
}

void Terminal::start()
{
    //Setup terminal session
    logInfo("Starting Terminal Mode.\n\r");
    char buffer[256];
    bool running = true;

    //Run terminal session
    while (running) {
        //Write terminal data to interface
        int terminalRead = terminal->readable();
        terminal->read(buffer, terminalRead);
        io->write(buffer, terminalRead);
        
        //Check for exit condition
        for (int i = 0; i < terminalRead; i++) {
            if (index < (exitMsg.size() - 1)) {
                if(buffer[i] == exitMsg[index]) {
                    index++;
                } else {
                    index = 0;
                }
            } else {
                running = false;
                wait(.1);
            }
        }
        
        //Write interface data to the terminal
        int ioRead = io->readable();
        io->read(buffer, ioRead);
        terminal->write(buffer, ioRead);
    }
    
    //Cleanup and return
    io->txClear();
    io->rxClear();
    logInfo("\n\rExited Terminal Mode.\n\r");
}
