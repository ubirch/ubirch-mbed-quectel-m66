/*
 * ubirch#1 M66 Modem AT command parser.
 *
 * @author Niranjan Rao
 * @date 2017-02-09
 *
 * @copyright &copy; 2015, 2016 ubirch GmbH (https://ubirch.com)
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

#include <cctype>
#include <fsl_rtc.h>
#include <string>
#include "mbed_debug.h"
#include "M66ATParser.h"
#include "M66Types.h"

#ifdef NCIODEBUG
#  define CIODUMP(buffer, size)
#  define CIODEBUG(...)
#  define CSTDEBUG(...)
#else
#  define CIODUMP(buffer, size) _debug_dump("GSM", buffer, size) /*!< Debug and dump a buffer */
#  define CIODEBUG(...)  printf(__VA_ARGS__)                     /*!< Debug I/O message (AT commands) */
#  define CSTDEBUG(...)  printf(__VA_ARGS__)                     /*!< Standard debug message (info) */
//#  define CSTDEBUG(fmt, ...) printf("%10.10s:%d::" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
//#  define CIODEBUG(fmt, ...) printf("%10.10s:%d::" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#endif

#define GSM_UART_BAUD_RATE 115200
#define RXTX_BUFFER_SIZE   512
#define MAX_SEND_BYTES     1400


M66ATParser::M66ATParser(PinName txPin, PinName rxPin, PinName rstPin, PinName pwrPin, bool debug)
    : _serial(txPin, rxPin, RXTX_BUFFER_SIZE), _powerPin(pwrPin), _resetPin(rstPin),  _packets(0), _packets_end(&_packets) {
    _serial.baud(GSM_UART_BAUD_RATE);
    _powerPin = 0;
}

bool M66ATParser::startup(void) {
    //When the board comes nack from deep sleep mode make sure the modem is restarted
    _powerPin = 0;
    wait_ms(200);
    _powerPin = 1;
    wait_ms(200);

    bool success = reset() && tx("AT+QIMUX=1") && rx("OK");
    return success;
}

bool M66ATParser::powerDown(void) {
    //TODO call this function if connection fails or on some unexpected events
    bool normalPowerDown = tx("AT+QPOWD=1") && rx("NORMAL POWER DOWN", 20);

    _powerPin =  0;

    return normalPowerDown;
}

bool M66ATParser::isModemAlive() {
    return (tx("AT") && rx("OK"));
}

int M66ATParser::checkGPRS() {
    int val = -1;
    if (!isModemAlive())
        return false;
    int ret = (tx("AT+CGATT?") && scan("+CGATT: %d", &val) && rx("OK", 10));
    return val;
}

bool M66ATParser::reset(void) {
    char response[4];

    bool modemOn = false;
    for (int tries = 0; !modemOn && tries < 3; tries++) {
        CSTDEBUG("M66 [--] !! reset (%d)\r\n", tries);
        // switch on modem
        _resetPin = 1;
        wait_ms(200);
        _resetPin = 0;
        wait_ms(1000);
        _resetPin = 1;

        if (isModemAlive()) return true;

        // TODO check if need delay here to wait for boot
        for (int i = 0; !modemOn && i < 1; i++) {
            modemOn = (tx("AT") && scan("%2s", &response)
                       && (!strncmp("AT", response, 2) || !strncmp("OK", response, 2)));

            wait_ms(500);
        }
    }

    if (modemOn) {
        // TODO check if the parser ignores any lines it doesn't expect
        modemOn = tx("ATE0") && scan("%3s", response)
                  && (!strncmp("ATE0", response, 3) || !strncmp("OK", response, 2))
                  && tx("AT+QIURC=1") && rx("OK")
                  && tx("AT+CMEE=1") && rx("OK");
/*TODO Do we need to save the setting profile
        tx("AT&W");
        rx("OK");*/
    }
    return modemOn;
}


bool M66ATParser::requestDateTime() {

    bool tdStatus = false;

    tdStatus = (tx("AT+QNITZ=1") && rx("OK", 10)
                && tx("AT+CTZU=1") && rx("OK", 10)
                && tx("AT+CFUN=1") && rx("OK", 10));

    bool connected = false;
    for (int networkTries = 0; !connected && networkTries < 20; networkTries++) {
        int bearer = -1, status = -1;
        if (tx("AT+CGREG?") && scan("+CGREG: %d,%d", &bearer, &status) && rx("OK", 15)) {
            // TODO add an enum of status codes
            connected = status == 1 || status == 5;
        }
        // TODO check if we need to use thread wait
        wait_ms(1000);
    }
    tdStatus &= (tx("AT+QNTP=\"pool.ntp.org\"") && rx("OK"));

    return tdStatus && connected;
}

bool M66ATParser::connect(const char *apn, const char *userName, const char *passPhrase) {
    // TODO implement setting the pin number, add it to the contructor arguments

    bool connected = false, attached = false;
    //TODO do we need timeout here
    for (int tries = 0; !connected && !attached && tries < 3; tries++) {

        // connecte to the mobile network
        for (int networkTries = 0; !connected && networkTries < 20; networkTries++) {
            int bearer = -1, status = -1;
            if (tx("AT+CREG?") && scan("+CREG: %d,%d", &bearer, &status) && rx("OK", 10)) {
                // TODO add an enum of status codes
                connected = status == 1 || status == 5;
            }
            // TODO check if we need to use thread wait
            wait_ms(1000);
        }
        if (!connected) continue;

        // attach GPRS
        if (!(tx("AT+QIDEACT") && rx("DEACT OK"))) continue;

        for (int attachTries = 0; !attached && attachTries < 20; attachTries++) {
            attached = tx("AT+CGATT=1") && rx("OK", 10);
            wait_ms(2000);
        }
        if (!attached) continue;

        // set APN and finish setup
        attached =
            tx("AT+QIFGCNT=0") && rx("OK") &&
            tx("AT+QICSGP=1,\"%s\",\"%s\",\"%s\"", apn, userName, passPhrase) && rx("OK", 10) &&
            tx("AT+QIREGAPP") && rx("OK", 10) &&
            tx("AT+QIACT") && rx("OK", 10);
    }

    // Send request to get the local time
    attached &= requestDateTime();

    return connected && attached ;
}

bool M66ATParser::disconnect(void) {
    return (tx("AT+QIDEACT") && rx("DEACT OK"));
}


const char *M66ATParser::getIPAddress(void) {
    if (!(tx("AT+QILOCIP") && scan("%s", _ip_buffer))) {
        return 0;
    }

    return _ip_buffer;
}

const char *M66ATParser::getIMEI() {
    if (!(tx("AT+GSN") && scan("%s", _imei))) {
        return 0;
    }
    return _imei;
}

bool M66ATParser::getLocation(char *lon, char *lat, rtc_datetime_t *datetime, int *zone) {

    char response[32] = "";

    string responseLon;
    string responseLat;

    // get location - +QCELLLOC: Longitude, Latitude
    if (!(tx("AT+QCELLLOC=1") && scan("+QCELLLOC: %s", response)))
        return false;

    string str(response);
    size_t found = str.find(",");
    if (found <= 0) return false;

    responseLon = str.substr(0, found - 1);
    responseLat = str.substr(found + 1);
    strcpy(lon, responseLon.c_str());
    strcpy(lat, responseLat.c_str());

    // get network time
    if (!((tx("AT+CCLK?")) && (scan("+CCLK: \"%d/%d/%d,%d:%d:%d+%d\"",
                                    &datetime->year, &datetime->month, &datetime->day,
                                    &datetime->hour, &datetime->minute, &datetime->second,
                                    &zone)))) {
        CSTDEBUG("M66 [--] !! no time received\r\n");
        return false;
    }

    datetime->year += 2000;

    CSTDEBUG("M66 [--] !! %d/%d/%d::%d:%d:%d::%d\r\n",
             datetime->year, datetime->month, datetime->day,
             datetime->hour, datetime->minute, datetime->second,
             *zone);
    return true;
}

bool M66ATParser::modem_battery(uint8_t *status, int *level, int *voltage) {
    return (tx("AT+CBC") && scan("+CBC: %d,%d,%d", status, level, voltage));
}

bool M66ATParser::isConnected(void) {
    return getIPAddress() != 0;
}

bool M66ATParser::queryIP(const char *url, const char *theIP) {

    for(int i = 0; i < 3; i++) {
        if((tx("AT+QIDNSGIP=\"%s\"", url)
             && rx("OK")
             && scan("%s", theIP))){
            if(strncmp(theIP, "ERROR", 5) != 0) return true;
        } wait(1);
    }
    return false;
}

bool M66ATParser::open(const char *type, int id, const char *addr, int port) {
    int id_resp = -1;

    //IDs only 0-5
    if (id > 6) {
        return false;
    }

    for(int i = 0; i < 3; i++) {

        /* opne a connection only if the QISTATE is IPINITAL, IP_CLOSE, IP STATUS
         * if it is in any other state then close the connection and / or deactivate context qideact
         */
        const int stateRet = queryConnection();

        if (stateRet == IP_INITIAL || stateRet == IP_CLOSE || stateRet == IP_STATUS) {

            if (!(tx("AT+QIDNSIP=0") && rx("OK"))) return false;

            if ((tx("AT+QIOPEN=%d,\"%s\",\"%s\",\"%d\"", id, type, addr, port)
                 && rx("OK", 10)
                 && scan("%d, CONNECT OK", &id_resp))) {
                return id == id_resp;
            }
        }
        /*TODO  AT+QIDEACT and QICLOSE, if open fails, check the application note*/
    }

    //TODO return a error code to debug the open fail in a bettwe way
    return false;
}

bool M66ATParser::send(int id, const void *data, uint32_t amount) {

    if (!(tx("AT+QISRVC=1") && rx("OK"))) return false;

    char *tempData = (char *) data;
    int remAmount = amount;
    int sendDataSize = 0;
    while (remAmount > 0) {
        sendDataSize = remAmount < MAX_SEND_BYTES ?  remAmount: MAX_SEND_BYTES;
        remAmount -= sendDataSize;

        /* TODO if this retry is required?
         * TODO May take a second try if device is busy
         * TODO use QISACK after you receive SEND OK, to check if whether the data has been sent to the remote
         */
        for (int i = 0; i < 2; i++) {
            if (tx("AT+QISEND=%d,%d", id, sendDataSize) && rx(">", 10)) {
                char cmd[512];
                while (flushRx(cmd, sizeof(cmd), 10)) {
                    CIODEBUG("GSM (%02d) !! '%s'\r\n", strlen(cmd), cmd);
                    checkURC(cmd);
                }
                CIODUMP((uint8_t *) tempData, (size_t)sendDataSize);
                if (_serial.write(tempData, (size_t)sendDataSize) >= 0 && rx("SEND OK", 20)) {
                    break;
                } else return false;
            } //if: AT+QISEND
        } //for:i
        tempData += sendDataSize;
    }//while
    return true;
}

/*TODO Use this commmand to get the IP status before running IP commands(open, send, ..)
 * getIPAddress() can also be used
 * A string parameter to indicate the status of the connection
 * "IP INITIAL"     :: The TCPIP stack is in idle state
 * "IP START"       :: The TCPIP stack has been registered
 * "IP CONFIG"      :: It has been start-up to activate GPRS/CSD context
 * "IP IND"         :: It is activating GPRS/CSD context
 * "IP GPRSACT"     :: GPRS/CSD context has been activated successfully
 * "IP STATUS"      :; The local IP address has been gotten by the command AT+QILOCIP
 * "TCP CONNECTING" :: It is trying to establish a TCP connection
 * "UDP CONNECTING" :: It is trying to establish a UDP connection
 * "IP CLOSE"       :: The TCP/UDP connection has been closed
 * "CONNECT OK"     :: The TCP/UDP connection has been established successfully
 * "PDP DEACT"      :: GPRS/CSD context was deactivated because of unknown reason
 */
int M66ATParser::queryConnection() {
    char resp[20];
    int qstate = -1;

    if (!(tx("ATV0") && rx("0"))) return false;

    bool ret = (tx("AT+QISTATE") && scan("%d", &qstate));

    scan("+QISTATE:0, %s", resp);
    scan("+QISTATE:1, %s", resp);
    scan("+QISTATE:2, %s", resp);
    scan("+QISTATE:3, %s", resp);
    scan("+QISTATE:4, %s", resp);
    scan("+QISTATE:5, %s", resp);
    rx("0");

    ret &= (tx("ATV1") && rx("OK"));

    if (!ret) return false;

    return qstate;
}

void M66ATParser::_packet_handler(const char *response) {
    int id;
    unsigned int amount;

    // parse out the packet
    if (sscanf(response, "+RECEIVE: %d, %d", &id, &amount) != 2) {
        return;
    }
    CSTDEBUG("M66 [%02d] -> %d bytes\r\n", id, amount);

    struct packet *packet = (struct packet *) malloc(sizeof(struct packet) + amount);
    if (!packet) {
        return;
    }

    packet->id = id;
    packet->len = (uint32_t) amount;
    packet->next = 0;

    const size_t bytesRead = read((char *) (packet + 1), (size_t) amount, 10);
    CIODUMP((uint8_t *) packet, (size_t) amount);
    if (bytesRead != amount) {
        CSTDEBUG("M66 [%02d] EE read(%d) != expected(%d)\r\n", id, bytesRead, amount);
        free(packet);
        return;
    }

    // append to packet list
    *_packets_end = packet;
    _packets_end = &packet->next;
}

int32_t M66ATParser::recv(int id, void *data, uint32_t amount) {
    Timer timer;
    timer.start();

    while (timer.read_ms() < _timeout) {
        CSTDEBUG("M66 [%02d] !! _timeout=%d, time=%d\r\n", id, (int) _timeout, (int) timer.read() * 1000);

        // check if any packets are ready for us
        for (struct packet **p = &_packets; *p; p = &(*p)->next) {
            if ((*p)->id == id) {
                struct packet *q = *p;

                if (q->len <= amount) { // Return and remove full packet
                    memcpy(data, q + 1, q->len);

                    if (_packets_end == &(*p)->next) {
                        _packets_end = p;
                    }
                    *p = (*p)->next;

                    uint32_t len = q->len;
                    free(q);
                    return len;
                } else { // return only partial packet
                    memcpy(data, q + 1, amount);

                    q->len -= amount;
                    memmove(q + 1, (uint8_t *) (q + 1) + amount, q->len);

                    return amount;
                }
            }
        }

        // Wait for inbound packet
        // TODO check what happens when we receive a packet (like OK)
        // TODO the response code may be different if connection is still open
        // TODO it may need to be moved to packet handler
        int receivedId;
        if (!scan("%d, CLOSED", &receivedId) && id == receivedId) {
            return -1;
        }
    }

    // timeout
    return -1;
}

bool M66ATParser::close(int id) {
    int id_resp;
    // TODO check if this retry is required
    //May take a second try if device is busy
    for (unsigned i = 0; i < 2; i++) {
        if (tx("AT+QICLOSE=%d", id) && scan("%d, CLOSE OK", &id_resp)) {
            return id == id_resp;
        }
    }

    return false;
}

void M66ATParser::setTimeout(uint32_t timeout_ms) {
    _timeout = timeout_ms;
}

bool M66ATParser::readable() {
    return (bool) _serial.readable();
}

bool M66ATParser::writeable() {
    return (bool) _serial.writeable();
}

void M66ATParser::attach(Callback<void()> func) {
    _serial.attach(func);
}

bool M66ATParser::tx(const char *pattern, ...) {
    char cmd[512];

    while (flushRx(cmd, sizeof(cmd), 10)) {
        CIODEBUG("GSM (%02d) !! '%s'\r\n", strlen(cmd), cmd);
        checkURC(cmd);
    }

    // cleanup the input buffer and check for URC messages
    cmd[0] = '\0';

    va_list ap;
    va_start(ap, pattern);
    vsnprintf(cmd, 512, pattern, ap);
    va_end(ap);

    _serial.puts(cmd);
    _serial.puts("\r\n");
    CIODEBUG("GSM (%02d) <- '%s'\r\n", strlen(cmd), cmd);

    return true;
}

int M66ATParser::scan(const char *pattern, ...) {
    char response[512];
    //TODO use if (readable()) here
    do {
        readline(response, 512 - 1, 10);
    } while (checkURC(response) != -1);

    va_list ap;
    va_start(ap, pattern);
    int matched = vsscanf(response, pattern, ap);
    va_end(ap);

    CIODEBUG("GSM (%02d) -> '%s' (%d)\r\n", strlen(response), response, matched);
    return matched;
}

bool M66ATParser::rx(const char *pattern, uint32_t timeout) {
    char response[512];
    size_t length = 0, patternLength = strnlen(pattern, sizeof(response));
    do {
        length = readline(response, 512 - 1, timeout);
        if (!length) return false;

        CIODEBUG("GSM (%02d) -> '%s'\r\n", strlen(response), response);
    } while (checkURC(response) != -1);

    return strncmp(pattern, (const char *) response, MIN(length, patternLength)) == 0;
}

int M66ATParser::checkURC(const char *response) {
    if (!strncmp("+RECEIVE:", response, 9)) {
        _packet_handler(response);
        return 0;
    }
    if (!strncmp("SMS Ready", response, 9)
        || !strncmp("Call Ready", response, 10)
        || !strncmp("+CPIN: READY", response, 12)
        || !strncmp("+QNTP: 0", response, 8)
        || !strncmp("+PDP DEACT", response, 10)
        || !strncmp("NORMAL POWER DOWN ", response, 18)

        ) {
        return 0;
    }

    return -1;
}

size_t M66ATParser::read(char *buffer, size_t max, uint32_t timeout) {
    Timer timer;
    timer.start();

    size_t idx = 0;
    while (idx < max && timer.read() < timeout) {
        if (!_serial.readable()) {
            __WFI();
            continue;
        }

        if (max - idx) buffer[idx++] = (char) _serial.getc();
    }

    return idx;
}

size_t M66ATParser::readline(char *buffer, size_t max, uint32_t timeout) {
    Timer timer;
    timer.start();

    size_t idx = 0;

    while (idx < max && timer.read() < timeout) {

        if (!_serial.readable()) {
            // nothing in the buffer, wait for interrupt
            __WFI();
            continue;
        }

        int c = _serial.getc();

        if (c == '\r') continue;

        if (c == '\n') {
            if (!idx) {
                idx = 0;
                continue;
            }
            break;
        }
        if (max - idx && isprint(c)) buffer[idx++] = (char) c;
    }

    buffer[idx] = 0;
    return idx;
}

size_t M66ATParser::flushRx(char *buffer, size_t max, uint32_t timeout) {
    Timer timer;
    timer.start();

    size_t idx = 0;

    do {
        for (int j = 0; j < (int) max && _serial.readable(); j++) {
            int c = _serial.getc();

            if (c == '\n' && idx > 0 && buffer[idx - 1] == '\r') {
                checkURC(buffer);
                idx = 0;
            } else if (max - idx && isprint(c)) {
                buffer[idx++] = (char) c;
            }
        }
        //TODO Do we actually need a timeout here
    } while (idx < max && _serial.readable() && timer.read() < timeout);

    buffer[idx] = 0;
    return idx;
}


void M66ATParser::_debug_dump(const char *prefix, const uint8_t *b, size_t size) {
    for (int i = 0; i < (int) size; i += 16) {
        if (prefix && strlen(prefix) > 0) printf("%s %06x: ", prefix, i);
        for (int j = 0; j < 16; j++) {
            if ((i + j) < (int) size) printf("%02x", b[i + j]); else printf("  ");
            if ((j + 1) % 2 == 0) putchar(' ');
        }
        putchar(' ');
        for (int j = 0; j < 16 && (i + j) < (int) size; j++) {
            putchar(b[i + j] >= 0x20 && b[i + j] <= 0x7E ? b[i + j] : '.');
        }
        printf("\r\n");
    }
}
