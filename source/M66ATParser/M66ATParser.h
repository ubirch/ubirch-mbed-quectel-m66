/*!
 * @file
 * @brief AT command parser for M66 modem.
 *
 * Contains functions for reading and writing from
 * the serial port
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
#ifndef M66ATPARSER_H
#define M66ATPARSER_H

#include "mbed.h"
#include <stdint.h>
#include <features/netsocket/nsapi_types.h>
#include <BufferedSerial/BufferedSerial.h>

/** M66 AT Parser Interface class.
    This is an interface to a M66 modem.
 */
class M66ATParser {
public:
    /** M66ATParser lifetime
     * @param tx        TX pin
     * @param rx        RX pin
     * @param rstPin    Reset pin
     * @param pwrPin    PowerKey pin
     * @param debug     Enable debugging
     */
    M66ATParser(PinName txPin, PinName rxPin, PinName rstPin, PinName pwrPin);

    /**
    * Startup the M66
    *
    * @return true only if M66 was started correctly
    */
    bool startup(void);

    /**
    * Reset M66
    *
    * @return true only if M66 resets successfully
    * play with PWERKEY - (only) to reset the modem, make sure the modem is reset and alive
    */
    bool reset(void);

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
    * Power down the modem using AT cmd and bring the power pin to low
    *
    * @return true if AT-powerDown was OK
    */
    bool powerDown(void);

    /**
    * Disconnect M66 from AP
    *
    * @return true only if M66 is disconnected successfully
    */
    bool disconnect(void);

    /**
    * Set up the NTP server and enable the M66 clock functions
    *
    * @return true if AT cmd were sucessful
    */
    bool requestDateTime(void);

    /**
    * Connect M66 to the network
    *
    * @param apn the address of the network APN
    * @param userName the user name
    * @param passPhrase the password
    * @return true only if M66 is connected successfully
    */
    bool connect(const char *apn, const char *userName, const char *passPhrase);

    /**
     * Get the IP address of M66
     *
     * @return null-teriminated IP address or null if no IP address is assigned
     */
    const char *getIPAddress(void);

    /**
     * Get the IP address of M66
     *
     * @return null-teriminated IP address or null if no IP address is assigned
     */
    bool getIMEI(char *getimei);

    /**
     * Get the Latitude, Longitude, Date and Time of the device
     *
     * @param lat latitude
     * @param lon longitude
     * @param datetime struct contains date and time
     * @return null-teriminated IP address or null if no IP address is assigned
     */
    bool getLocation(char *lon, char *lat);

    bool getNetworkTime(tm *datetime, int *zone);

    bool getUnixTime(time_t *t);

    /**
     * Get the Battery status, level and voltage of the device
     *
     * @param status battery status
     * @param level battery level
     * @param voltage battery voltage
     * @return return false if
     */
    bool modem_battery(uint8_t *status, int *level, int *voltage);

    /**
    * Check if M66 is connected
    *
    * @return true only if the chip has an IP address
    */
    bool isConnected(void);

    /**
    * Get the IP of the host
    *
    * @return true only if the chip has an IP address
    */
    bool queryIP(const char *url, const char *theIP);

    /**
    * Open a socketed connection
    *
    * @param type the type of socket to open "UDP" or "TCP"
    * @param id id to give the new socket, valid 0-4
    * @param port port to open connection with
    * @param addr the IP address of the destination
    * @return true only if socket opened successfully
    */
    bool open(const char *type, int id, const char *addr, int port);

    /**
    * Sends data to an open socket
    * 1046 Bytes can be sent each time
    *
    * @param id id of socket to send to
    * @param data data to be sent
    * @param amount amount of data to be sent - max 1024
    * @return true only if data sent successfully
    */
    bool send(int id, const void *data, uint32_t amount);

    /**
    * Get the M66 connection status
    *
    * @return status
    */
    int queryConnection();

    /**
    * Receives data from an open socket
    *
    * @param id id to receive from
    * @param data placeholder for returned information
    * @param amount number of bytes to be received
    * @return the number of bytes received
    */
    int32_t recv(int id, void *data, uint32_t amount);

    /**
    * Closes a socket
    *
    * @param id id of socket to close, valid only 0-4
    * @return true only if socket is closed successfully
    */
    bool close(int id);

    /**
    * Allows timeout to be changed between commands
    *
    * @param timeout_ms timeout of the connection
    */
    void setTimeout(uint32_t timeout_ms);

    /**
    * Checks if data is available
    */
    bool readable();

    /**
    * Checks if data can be written
    */
    bool writeable();

    /**
    * Attach a function to call whenever network state has changed
    *
    * @param func A pointer to a void function, or 0 to set as none
    */
    void attach(Callback<void()> func);

    /**
    * Attach a function to call whenever network state has changed
    *
    * @param obj pointer to the object to call the member function on
    * @param method pointer to the member function to call
    */
    template<typename T, typename M>
    void attach(T *obj, M method) {
        attach(Callback<void()>(obj, method));
    }

    /*! send a command */
    bool tx(const char *pattern, ...);

    /**
    * @brief Expect a formatted response, blocks until the response is received or timeout.
    * This function will ignore URCs and return when the first non-URC has been received.
    * @param pattern the pattern to match
    * @return the number of matched elements
    */
    int scan(const char *pattern, ...);

    /*!
    * @brief Expect a certain response, blocks util the response received or timeout.
    * This function will ignore URCs and return when the first non-URC has been received.
    * @param pattern the string to expect
    * @return true if received or false if not
    */
    bool rx(const char *pattern, uint32_t timeout = 5);

    /*!
    * Check if this line is an unsolicited result code.
    * @param response  the pattern to match
    * @return the code index or -1 if it is no known code
    */
    int checkURC(const char *response);

    /*!
    * @brief Read a single line from the M66
    * @param buffer the character line buffer to read into
    * @param max the number of characters to read
    * @return the number of characters read
    */
    size_t readline(char *buffer, size_t max, uint32_t timeout);

    /*!
    * @brief Read binary data into a buffer
    * @param buffer the buffer to read into
    * @param max the number of bytes to read
    * @return the amount of bytes read
    */
    size_t read(char *buffer, size_t max, uint32_t timeout = 5);

    size_t flushRx(char *buffer, size_t max, uint32_t timeout = 5);

private:
    BufferedSerial _serial;

    DigitalOut _powerPin;
    DigitalOut _resetPin;
    struct packet {
        struct packet *next;
        int id;
        uint32_t len;
        // data follows
    } *_packets, **_packets_end;

    void _packet_handler(const char *response);

    void _debug_dump(const char *prefix, const uint8_t *b, size_t size);

    bool networkTimeSynchronised;
    int _timeout;
    char _ip_buffer[16];
    char _imei[16];

};

#endif
