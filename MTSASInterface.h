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
 
#ifndef MTSAS_INTERFACE_H
#define MTSAS_INTERFACE_H

#include "mbed.h"
#include "ATParser.h" 
#define MTSAS_SOCKET_COUNT 6

struct gps_data{
    char latitude[25];
    char longitude[25];
    char UTC[25];
    char altitude[25];
};

 
/** MTSASInterface class
 *  Implementation of the NetworkInterface for MTSAS 
 */
class MTSASInterface : public NetworkStack, public CellularInterface
{
public:
    /** MTSASInterface
     * @param tx      TX for radio communication
     * @param rx      RX  for radio communication
     * @param debug   Print out AT comands
     * @param on_batt boolean for when the board is on the battery pack
     */
    MTSASInterface(PinName tx, PinName rx, bool debug=false);

    ~MTSASInterface();
    /** Set the cellular network APN and credentials
     *
     *  @param apn      Optional name of the network to connect to
     *  @param user     Optional username for the APN
     *  @param pass     Optional password fot the APN
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t set_credentials(const char *apn = 0,
            const char *username = 0, const char *password = 0);
 
    /** Start the interface
     *
     *  @param apn      Optional name of the network to connect to
     *  @param username Optional username for your APN
     *  @param password Optional password for your APN 
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t connect(const char *apn,
            const char *username = 0, const char *password = 0);
 
    /** Start the interface
     *
     *  Attempts to connect to a cellular network based on supplied credentials
     *
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t connect();

    /** Stop the interface
     *
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t disconnect();
 
    /** Get the internally stored IP address
     *  @return             IP address of the interface or null if not yet connected
     */
    virtual const char *get_ip_address();
 
    /** Get the internally stored MAC address
     *  @return             MAC address of the interface
     */
    virtual const char *get_mac_address();
 
    nsapi_error_t gethostbyname(const char* name, SocketAddress *address, nsapi_version_t version);

    /** Get the gps location of the device
     *  @param lat_default  the default latitude if gps module fails to get a fix
     *  @param lon_default  the defauly longitude if gps module fails to get a fix
     *  @return             struct of type gps_data
     *  @note               Coordinates will be returned in the ISO 6709 format
     */
    virtual gps_data get_gps_location(const char* lat_default="None", const char* lon_default="None");   
    
    /** Get the imei of the device
     *  @param imei the buffer in which to store the imei number
     */
    virtual void get_imei(char* imei);
    
    /** Attach a function to be called when a text is recevieds
     *  @param callback  function pointer to a callback that will accept the message 
     *  contents when a text is received.
     */
    virtual void sms_attach(void (*callback)(char *));

protected:
    virtual bool set_gps_state(int state);
    virtual int get_gps_state();
  

    /** Provide access to the NetworkStack object
     *
     *  @return The underlying NetworkStack object
     */
    virtual NetworkStack *get_stack();

    /** Open a socket
     *  @param handle       Handle in which to store new socket
     *  @param proto        Type of socket to open, NSAPI_TCP or NSAPI_UDP
     *  @return             0 on success, negative on failure
     */
    virtual int socket_open(void **handle, nsapi_protocol_t proto);
 
    /** Close the socket
     *  @param handle       Socket handle
     *  @return             0 on success, negative on failure
     *  @note On failure, any memory associated with the socket must still 
     *        be cleaned up
     */
    virtual int socket_close(void *handle);
 
    /** Bind a server socket to a specific port
     *  @param handle       Socket handle
     *  @param address      Local address to listen for incoming connections on 
     *  @return             0 on success, negative on failure.
     */
    virtual int socket_bind(void *handle, const SocketAddress &address);
 
    /** Start listening for incoming connections
     *  @param handle       Socket handle
     *  @param backlog      Number of pending connections that can be queued up at any
     *                      one time [Default: 1]
     *  @return             0 on success, negative on failure
     */
    virtual int socket_listen(void *handle, int backlog);
 
    /** Connects this TCP socket to the server
     *  @param handle       Socket handle
     *  @param address      SocketAddress to connect to
     *  @return             0 on success, negative on failure
     */
    virtual int socket_connect(void *handle, const SocketAddress &address);
 
    /** Accept a new connection.
     *  @param handle       Handle in which to store new socket
     *  @param server       Socket handle to server to accept from
     *  @return             0 on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_accept(nsapi_socket_t server,
            nsapi_socket_t *handle, SocketAddress *address=0);
 
    /** Send data to the remote host
     *  @param handle       Socket handle
     *  @param data         The buffer to send to the host
     *  @param size         The length of the buffer to send
     *  @return             Number of written bytes on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_send(void *handle, const void *data, unsigned size);
 
    /** Receive data from the remote host
     *  @param handle       Socket handle
     *  @param data         The buffer in which to store the data received from the host
     *  @param size         The maximum length of the buffer
     *  @return             Number of received bytes on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_recv(void *handle, void *data, unsigned size);
 
    /** Send a packet to a remote endpoint
     *  @param handle       Socket handle
     *  @param address      The remote SocketAddress
     *  @param data         The packet to be sent
     *  @param size         The length of the packet to be sent
     *  @return the         number of written bytes on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_sendto(void *handle, const SocketAddress &address, const void *data, unsigned size);
 
    /** Receive a packet from a remote endpoint
     *  @param handle       Socket handle
     *  @param address      Destination for the remote SocketAddress or null
     *  @param buffer       The buffer for storing the incoming packet data
     *                      If a packet is too long to fit in the supplied buffer,
     *                      excess bytes are discarded
     *  @param size         The length of the buffer
     *  @return the         number of received bytes on success, negative on failure
     *  @note This call is not-blocking, if this call would block, must
     *        immediately return NSAPI_ERROR_WOULD_WAIT
     */
    virtual int socket_recvfrom(void *handle, SocketAddress *address, void *buffer, unsigned size);
 
    /** Register a callback on state change of the socket
     *  @param handle       Socket handle
     *  @param callback     Function to call on state change
     *  @param data         Argument to pass to callback
     *  @note Callback may be called in an interrupt context.
     */
    virtual void socket_attach(void *handle, void (*callback)(void *), void *data);
    
    virtual bool registered();
    virtual bool set_ip_addr();    
    virtual nsapi_error_t init();

		
private:
    int context;                            // CELL PDP context
    // AT Parser variables
    bool _debug;                            // debug print for AT parser
    BufferedSerial _serial;                 // Serial object for parser to communicate with radio
    ATParser _parser;                       // Send AT commands and parse responses
    Thread event_thread;                    // Thread to poll for SRING indicating incoming socket data
    Thread sms_event_thread;
    Mutex at_mutex;                         // Mutex that only allows one thread at a time to execute AT Commands
    SocketAddress _ip_address;              // Local IP address
    Semaphore rx_sem;                       // Semphore to signal event_thread to check SRING
    Semaphore sms_rx_sem;                   // Semaphore to signal sms_event_thread to check for incoming text 
    void sms_rx_sem_release();              // Release the sms semaphore on serial RX
    void sms_listen();                      // Configure device to listen for text messages 
    void handle_sms_event();                // Handle incoming text data
    char _mac_address[NSAPI_MAC_SIZE];      // local Mac
    char _pin[sizeof("1234")];              // Cell pin
    void event();                           // Event signifying socket rcv data 	
    void handle_event();                    // To be used by event_thread to check SRING 
    void rx_sem_release();                  // To attached to buffered serial to signal thread that RX on serial line
    bool _socket_ids[MTSAS_SOCKET_COUNT];   // array of available sockets
    struct {
        void (*callback)(void *);
        void *data;
    } _cbs[MTSAS_SOCKET_COUNT];             // Callbacks for socket_attach 
    DigitalOut reset;                       // Set RESET - Set the hardware reset line to the radio 
    void (*_sms_cb)(char *);                // Callback when text message is received 
};

#endif
