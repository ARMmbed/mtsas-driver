/* C027 implementation of NetworkInterfaceAPI
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "MTSASInterface.h"
 
 
/** C027Interface class
 *  Implementation of the NetworkInterface for C027
 */

MTSASInterface::MTSASInterface(const char *simpin, bool debug, bool on_batt)
    : _debug(debug), bc_nce(PB_2)
{
    bc_nce = (int)on_batt;
    strcpy(_pin, simpin);
    _cbs.callback = NULL;
    _cbs.data = NULL;
}
nsapi_error_t MTSASInterface::set_credentials(const char *apn,
    const char *username , const char *password)
{
    if (radio->setApn(apn) != MTS_SUCCESS)
        return NSAPI_ERROR_NO_CONNECTION;
    return 0;
}

nsapi_error_t MTSASInterface::connect(const char *apn,
            const char *username, const char *password)
{
    Serial debug(USBTX, USBRX);
    debug.baud(9600);
    
    //Sets the log level to INFO, higher log levels produce more log output.
    //Possible levels: NONE, FATAL, ERROR, WARNING, INFO, DEBUG, TRACE
    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);
    
    logInfo("initializing cellular radio");
    io = new mts::MTSSerialFlowControl(RADIO_TX, RADIO_RX, RADIO_RTS, RADIO_CTS);
    if (! io)
        return NSAPI_ERROR_DEVICE_ERROR;
    logInfo("Serial flow");
    // radio default baud rate is 115200
    io->baud(115200);
    radio = mts::CellularFactory::create(io);
    type = (mts::Cellular::Radio)(radio->getRadioType());
    if (! radio)
        return NSAPI_ERROR_DEVICE_ERROR;
    logInfo("Radio");
    // Transport must be set properly before any TCPSocketConnection or UDPSocket objects are created
    Transport::setTransport(radio);
    if(set_credentials(apn, username, password)){
        return NSAPI_ERROR_NO_CONNECTION;
    }
    logInfo("Transport");
    return connect();
}
 
nsapi_error_t MTSASInterface::connect()
{
    if (!radio->connect())
        return NSAPI_ERROR_NO_CONNECTION;
    logInfo("Connect");
    return 0;
}
nsapi_error_t MTSASInterface::disconnect() 
{
    radio->disconnect();
    return 0;
}
const char *MTSASInterface::get_ip_address()
{
    _ip_address.set_ip_address(radio->getDeviceIP().c_str());
    return _ip_address.get_ip_address();
}
const char *MTSASInterface::get_mac_address()
{
    return 0;
}
 
NetworkStack *MTSASInterface::get_stack()
{
    return this;
}

struct mtsas_socket {
    nsapi_protocol_t proto;
    bool connected;
    int port;
};

int MTSASInterface::socket_open(void **handle, nsapi_protocol_t proto)
{
    struct mtsas_socket *socket = new struct mtsas_socket;
    socket->port = 1;
    socket->proto = proto;
    socket->connected = false;
    *handle = socket;
    return 0;
}

int MTSASInterface::socket_close(void *handle)
{
    struct mtsas_socket *socket = (struct mtsas_socket *)handle;   
    bool success = radio->close(false);
    socket->connected = !success;
    if (!success)
        return NSAPI_ERROR_DEVICE_ERROR;
    return 0;
}

int MTSASInterface::socket_bind(void *handle, const SocketAddress &address)
{
    return NSAPI_ERROR_UNSUPPORTED;}
 
int MTSASInterface::socket_listen(void *handle, int backlog)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int MTSASInterface::socket_connect(void *handle, const SocketAddress &address)
{
    struct mtsas_socket *socket = (struct mtsas_socket *)handle;   
    uint16_t typeSocket = (socket->proto == NSAPI_UDP) ? 1 : 0;
    bool success = radio->open(address.get_ip_address(), address.get_port(), (mts::Cellular::Mode)typeSocket);
    socket->connected = success;
    if (success){
        socket->port = address.get_port();
        return 0;
    }
    return NSAPI_ERROR_DEVICE_ERROR;
}
 
int MTSASInterface::socket_accept(nsapi_socket_t server,
            nsapi_socket_t *handle, SocketAddress *address)
{
    return NSAPI_ERROR_UNSUPPORTED;
}
 
int MTSASInterface::socket_send(void *handle, const void *data, unsigned size)
{

    return radio->write((char *)data, size);
}

int MTSASInterface::socket_recv(void *handle, void *data, unsigned size)
{
    return radio->read((char *)data, size, 1000);
}

int MTSASInterface::socket_sendto(void *handle, const SocketAddress &address, const void *data, unsigned size)
{
     struct mtsas_socket *socket = (struct mtsas_socket *)handle;
     if (!socket->connected) {
        int err = socket_connect(socket, address);
        if (err < 0) {
            return err;
        }
    }
    return socket_send(socket, data, size);
}

int MTSASInterface::socket_recvfrom(void *handle, SocketAddress *address, void *buffer, unsigned size)
{
    struct mtsas_socket *socket = (struct mtsas_socket *)handle;   
    return socket_recv(socket, (char *)buffer, size);
}

void MTSASInterface::socket_attach(void *handle, void (*callback)(void *), void *data)
{
    _cbs.callback = callback;
    _cbs.data = data;
}

void MTSASInterface::event() {
    if (_cbs.callback) {
        _cbs.callback(_cbs.data);
    }
}
 