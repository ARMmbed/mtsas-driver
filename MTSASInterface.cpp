/* MTSAS implementation of NetworkInterfaceAPI
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
 
/** MTSASInterface class
 *  Implementation of the NetworkInterface for MTS DRAGONFLY
 */

enum registration_status {  NOT_REGISTERED = 0, 
                            REGISTERED = 1, 
                            SEARCHING = 2, 
                            DENIED = 3, 
                            UNKNOWN = 4, 
                            ROAMING = 5};

MTSASInterface::MTSASInterface(PinName tx, PinName rx, bool debug)
    : _serial(tx, rx, 1024), _parser(_serial), reset(PinName(RESET))
{
    _parser.debugOn(debug);
    _debug = debug;

    _serial.baud(115200);
    _parser.setTimeout(3000);
    _serial.attach(this, &MTSASInterface::event);
    memset(_socket_ids, 0 , sizeof(_socket_ids));
    memset(_cbs, 0, sizeof(_cbs));
    //PDP context
    context = 1;
}
MTSASInterface::~MTSASInterface(){

}

nsapi_error_t MTSASInterface::set_credentials(const char *apn,
    const char *username , const char *password)
{
    for (int i=1; i < MTSAS_SOCKET_COUNT; i++){
        //Socket configuration 
        //AT#SCFG=<socket id>,<PDP context>,<packet size default 300>,
                //<exchange timeout>,<connection to>,<txto>
        _parser.send("AT#SCFG=%d,%d,0,0,600,0",i,context);
        _parser.recv("OK");
    }
    //Activate the PDP context 

    if (_parser.send("AT+CGDCONT=%d,\"IP\",\"%s\"", context, apn) && _parser.recv("OK"))
        return 0;
    return NSAPI_ERROR_DEVICE_ERROR;
}

nsapi_error_t MTSASInterface::init()
{
    _parser.setTimeout(10000);

    //Reboot the chip
    _parser.send("AT#REBOOT");
    _parser.recv("OK");
    _parser.setTimeout(3000);
    //Wait for response after reboot
    for (int i = 0; i < 10; i++){
        if (_parser.send("AT") && _parser.recv("OK")){
            break;
        }   
    }

    //Device name
    _parser.send("AT+CGMM");
    _parser.recv("OK");
    _parser.send("AT+CGMM");
    _parser.recv("OK");
    return 0;
}

nsapi_error_t MTSASInterface::connect(const char *apn,
            const char *username, const char *password)
{
    return (init() || set_credentials(apn) || connect()) ? NSAPI_ERROR_NO_CONNECTION : 0;
}

bool MTSASInterface::registered()
{
    //Get the network registation
    int stat = NOT_REGISTERED;

    int RETRIES = 20;
    _parser.send("AT+CREG?");
    _parser.recv("+CREG:%*d,%d", &stat);
    _parser.recv("OK"); 
    //Keep trying if we are searching for a registration
    while(stat == SEARCHING){
    	_parser.send("AT+CREG?");
    	_parser.recv("+CREG:%*d,%d", &stat);
    	_parser.recv("OK");
   	}
    return (stat == REGISTERED || stat == ROAMING);
}
bool MTSASInterface::set_ip_addr()
{
    char* ip_buff = (char*)malloc(256);
    bool res = false;   

    //Try a few times to get an IP address 
    for (int i=0; i<5; i++){
        res  = _parser.send("AT#SGACT=%d,1", context) && 
               _parser.recv("#SGACT: %s%*[\r]%*[\n]", ip_buff) &&
               _parser.recv("OK");
        if(res)
            break;
    }
 
    res = res && _ip_address.set_ip_address(ip_buff);
    free(ip_buff);
    return res;
}
 
nsapi_error_t MTSASInterface::connect()
{
    if (!registered() || !set_ip_addr()){
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    return 0;
}
nsapi_error_t MTSASInterface::disconnect() 
{
    //Deactivate PDP context (frees any network resources associated with context)
    return (_parser.send("AT#SGACT=%d,0",context) && _parser.recv("OK")) ? 0 : NSAPI_ERROR_DEVICE_ERROR; 
}
const char *MTSASInterface::get_ip_address()
{
    if(_ip_address.get_ip_address() == NULL){
        set_ip_addr();
    }
    return _ip_address.get_ip_address();
}
const char *MTSASInterface::get_mac_address()
{
    return 0;
}

nsapi_error_t MTSASInterface::gethostbyname(const char* name, SocketAddress *address, nsapi_version_t version)
{ 
    char* ip_buff = (char*)malloc(256);
    //Execute DNS query

    if (!_parser.send("AT#QDNS=%s",name) || !_parser.recv("#QDNS:%*[^,],\"%[^\"]\"%*[\r]%*[\n]", ip_buff)){
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    address->set_ip_address(ip_buff);

    free(ip_buff);

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
    int id;
	  SocketAddress addr;
};

int MTSASInterface::socket_open(void **handle, nsapi_protocol_t proto)
{
    // Look for unused socket
    int id = -1;
    for (int i = 0; i < MTSAS_SOCKET_COUNT; i++) {
        if (!_socket_ids[i]){
            // IDS 1-6 valid
            id = i+1;
            _socket_ids[i] = true;
            break;
        }
    }
    if (id == -1){
        return NSAPI_ERROR_NO_SOCKET;
    }
    struct mtsas_socket *socket = new struct mtsas_socket;
    socket->id = id;
    socket->port = 1;
    socket->proto = proto;
    socket->connected = false;
    *handle = socket;
    return 0;
}

int MTSASInterface::socket_close(void *handle)
{
    struct mtsas_socket *socket = (struct mtsas_socket *)handle;
    //Issue socket close command

    if (_parser.send("AT#SH=%d",socket->id) && _parser.recv("OK")){
        _socket_ids[socket->id-1] = false;
        delete socket;
        return 0;
    }
    return NSAPI_ERROR_DEVICE_ERROR;
}

int MTSASInterface::socket_bind(void *handle, const SocketAddress &address)
{
    return NSAPI_ERROR_UNSUPPORTED;
}
 
int MTSASInterface::socket_listen(void *handle, int backlog)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int MTSASInterface::socket_connect(void *handle, const SocketAddress &address)
{
    struct mtsas_socket *socket = (struct mtsas_socket *)handle;   
    if (socket->connected){
        return 0;
    }
    uint16_t typeSocket = (socket->proto == NSAPI_UDP) ? 1 : 0;
    //Socket dial SD=[socket id], [UDP or TCP], [Remote port], [Remote addr]
    if(_parser.send("AT#SD=%d,%d,%d,\"%s\",0,1,1", socket->id, typeSocket, 
       address.get_port(), address.get_ip_address()) &&
       _parser.recv("OK")){
            socket->connected = true;

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

    struct mtsas_socket *socket = (struct mtsas_socket *)handle;   
    int amnt_sent = -1;

    if(_parser.send("AT#SSENDEXT=%d,%d",socket->id, size)){
        //OK to write message
        _parser.recv("> ");
        amnt_sent = _parser.write((char *)data, (int)size);
        _parser.recv("OK");
    }
    return amnt_sent;
}

int MTSASInterface::socket_recv(void *handle, void *data, unsigned size)
{
    struct mtsas_socket *socket = (struct mtsas_socket *)handle;   
    int amnt_rcv = -1;
    uint16_t typeSocket = (socket->proto == NSAPI_UDP) ? 1 : 0;

    int recv_size = 0;
    //Issue send command SRECV=[socket id], [# bytes to recv]
    if(_parser.send("AT#SRECV=%d,%d",socket->id, size) && _parser.recv("#SRECV:%d,%d%*[\r]%*[\n]", socket->id, &recv_size)){
        amnt_rcv = _parser.read((char *)data, (int)recv_size);
        _parser.recv("OK");
    }
    if (amnt_rcv == -1) {
        if (_debug){
            //See socket information
            int received = 0;
            int buff_in = 0;
            int sent = 0;
            int ack_waiting = 0;
            _parser.send("AT#SI=%d", socket->id);
            _parser.recv("#SI: %*d,%d,%d,%d,%d", &sent, &received, &buff_in, &ack_waiting);
            _parser.recv("OK");
        }

        return NSAPI_ERROR_WOULD_BLOCK;
    }
    return amnt_rcv;
}

int MTSASInterface::socket_sendto(void *handle, const SocketAddress &address, const void *data, unsigned size)
{
    struct mtsas_socket *socket = (struct mtsas_socket *)handle;
    
		if (socket->connected && socket->addr != address) {
        if (socket_close(&handle) != 0) {
            return NSAPI_ERROR_DEVICE_ERROR;
        }
        socket->connected = false;
		}
	
		if (!socket->connected) {
        int err = socket_connect(socket, address);
        if (err < 0) {
            return err;
        }
				socket->addr = address;
		}
    return socket_send(socket, data, size);
}

int MTSASInterface::socket_recvfrom(void *handle, SocketAddress *address, void *buffer, unsigned size)
{
    struct mtsas_socket *socket = (struct mtsas_socket *)handle;   
    int ret = socket_recv(socket, (char *)buffer, size);
		if (ret >= 0 && address) {
        *address = socket->addr;
    }
    return ret;
}

void MTSASInterface::socket_attach(void *handle, void (*callback)(void *), void *data)
{
    struct mtsas_socket *socket = (struct mtsas_socket *)handle;   
    _cbs[socket->id-1].callback = callback;
    _cbs[socket->id-1].data = data;
}

void MTSASInterface::event() {
    for (int i = 0; i < MTSAS_SOCKET_COUNT; i++){
        if (_cbs[i].callback) {
            _cbs[i].callback(_cbs[i].data);
        }
    }
}
 
