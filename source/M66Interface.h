/*!
 * @file
 * @brief Interface for M66 core functionality.
 *
 * Contains functions for enabling and disabling modem
 * and establishing IP connection
 *
 * @author Niranjan Rao
 * @date 2017-02-09
 *
 * @copyright &copy; 2015, 2016, 2017 ubirch GmbH (https://ubirch.com)
 *
 * ```
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
 * ```
 */

#ifndef M66_INTERFACE_H
#define M66_INTERFACE_H

#include "mbed.h"
#include "fsl_rtc.h"
#include "M66ATParser.h"

#define M66_SOCKET_COUNT 5

/** M66Interface class
 *  Implementation of the NetworkStack for the M66 GSM Modem
 */
class M66Interface : public NetworkStack, public CellularBase {
public:
    /** M66Interface lifetime
     * @param tx        TX pin
     * @param rx        RX pin
     * @param rstPin    Reset pin
     * @param pwrPin    PowerKey pin
     * @param debug     Enable debugging
     */
    M66Interface(PinName tx, PinName rx, PinName rstPin, PinName pwrPin);

    /**
    * Startup the M66
    *
    * @return true only if M66 was started correctly
    */
    bool powerUpModem();

    /**
    * Reset M66
    *
    * @return true only if M66 resets successfully
    * play with PWERKEY - (only) to reset the modem, make sure the modem is reset and alive
    */
    bool reset();

    /**
    * Power down the modem using AT cmd and bring the power pin to low
    *
    * @return true if AT-powerDown was OK
    */
    bool powerDown();

    /**
    * Check if the Modem is poweredup and running
    *
    * @return true only if M66 OK's to AT cmd
    */
    bool isModemAlive();

    /**
    * Check the modem GPRS status
    *
    * @return 0: GPRS is detached; 1: GPRS is attached
    */
    int checkGPRS();

    /**
    * Connect M66 to the network
    *
    */
    virtual int connect();

    /** Start the interface
     *
     *  Attempts to connect to a WiFi network.
     *
     *  @param ssid      Name of the network to connect to
     *  @param pass      Security passphrase to connect to the network
     *  @param security  Type of encryption for connection (Default: NSAPI_SECURITY_NONE)
     *  @param channel   This parameter is not supported, setting it to anything else than 0 will result in NSAPI_ERROR_UNSUPPORTED
     *  @return          0 on success, or error code on failure
     */
    virtual int connect(const char *apn, const char *userName, const char *passPhrase);

    /** Set the GSM Modem network credentials
     *
     *  @param apn        Address of the netwok Access Point Name
     *  @param userName   User name
     *  @param passPhrase The password
     *  @return           0 on success, or error code on failure
     */
    virtual void set_credentials(const char *apn, const char *userName, const char *passPhrase);

    /** Stop the interface
     *  @return             0 on success, negative on failure
     */
    virtual int disconnect();

    /** Get the internally stored IP address
     *  @return             IP address of the interface or null if not yet connected
     */
    virtual const char *get_ip_address();

    /** Set the IMEI
     *  @return             true if IMEI is set sucessfully
     */
    int set_imei();

   /** Get the internally stored IMEI
    *  @return             IMEI of the Device or null if not yet Powered on
    */
    const char *get_imei();

    /**
     * Get the Latitude, Longitude, Date and Time of the device
     *
     * @param lat latitude
     * @param lon longitude
     * @param datetime struct contains date and time
     * @return null-teriminated IP address or null if no IP address is assigned
     */
    bool get_location(char *lon, char *lat);

    bool getDateTime(tm * dateTime, int * zone);

    bool getUnixTime(time_t *t);

    bool queryIP(const char *url, const char *theIP);

    /**
     * Get the Battery status, level and voltage of the device
     *
     * @param status battery status
     * @param level battery level
     * @param voltage battery voltage
     * @return return false if
     */
    bool getModemBattery(uint8_t *status, int *level, int *voltage);

    /** Translates a hostname to an IP address with specific version
     *
     *  The hostname may be either a domain name or an IP address. If the
     *  hostname is an IP address, no network transactions will be performed.
     *
     *  If no stack-specific DNS resolution is provided, the hostname
     *  will be resolve using a UDP socket on the stack.
     *
     *  @param address  Destination for the host SocketAddress
     *  @param host     Hostname to resolve
     *  @param version  IP version of address to resolve, NSAPI_UNSPEC indicates
     *                  version is chosen by the stack (defaults to NSAPI_UNSPEC)
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t gethostbyname(const char *host, SocketAddress *address, nsapi_version_t version);

    /** Add a domain name server to list of servers to query
     *
     *  @param addr     Destination for the host address
     *  @return         0 on success, negative error code on failure
     */
    using NetworkInterface::add_dns_server;

protected:
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
    virtual int socket_accept(void *handle, void **socket, SocketAddress *address);

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
     *  @return             The number of written bytes on success, negative on failure
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
     *  @return             The number of received bytes on success, negative on failure
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

    /** Provide access to the NetworkStack object
     *
     *  @return The underlying NetworkStack object
     */
    virtual NetworkStack *get_stack() {
        return this;
    }

private:
    M66ATParser _m66;
    bool _sockets[M66_SOCKET_COUNT];

    char _apn[10];
    char _userName[10];
    char _passPhrase[10];
    char _imei[16];

    void event();

    struct {
        void (*callback)(void *);

        void *data;
    } _cbs[M66_SOCKET_COUNT];
};

#endif
