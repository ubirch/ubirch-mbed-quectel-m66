/*
 * Example program establishes TCP/IP connection,
 * and sends a HTTP GET request
 */

#if 0
#include "mbed.h"
#include "../mbed-os-quectelM66-driver/M66Interface.h"
#include "../config.h"

M66Interface modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER, true);
DigitalOut led(LED1);

static const char *const message_template = "POST /api/avatarService/v1/device/update HTTP/1.1\r\n"
        "Host: api.demo.dev.ubirch.com:8080\r\n"
        "Content-Length: 120\r\n"
        "\r\n"
        "{\"v\":\"0.0.0\",\"a\":\"%s\",\"p\":{\"t\":1}}";

static const char *const tempMessage = "GET / HTTP/1.1\r\n\r\n";

void sendData(void) {

    /*use socket ret values to do further steps, print enums or what error means */

    int failCount = 0;

    while (failCount < 5) {
        TCPSocket socket;
        printf("Sending new data\r\n");

        int ret;
        const char *theUrl = "www.arm.com";
        char theIP[20];

        // Open a socket on the network interface, and create a TCP connection to www.arm.com
        socket.open(&modem);
        socket.set_timeout(0);
        bool ipret = modem.queryIP(theUrl, theIP);

        if (ipret) {
            // http://api.demo.dev.ubirch.com:8080/api/avatarService/v1/device/update
            ret = socket.connect(theIP, 80);

            if (ret >= 0) {

                int message_size = snprintf(NULL, 0, message_template, imeiHash);
                char *message = (char *) malloc((size_t) (message_size + 1));
                sprintf(message, message_template, imeiHash);

                int r = socket.send(tempMessage, (nsapi_size_t) strlen(tempMessage));
                if (r > 0) {
                    // Recieve a simple http response and print out the response line
                    char buffer[64];
                    r = socket.recv(buffer, sizeof(buffer));
                    if (r >= 0) {
                        printf("received %d bytes\r\n---\r\n%.*s\r\n---\r\n", r,
                               (int) (strstr(buffer, "\r\n") - buffer),
                               buffer);
                    } else {
                        printf("receive failed: %d\r\n", r);
                    }
                } else {
                    printf("send failed: %d\r\n", r);
                }

                free(message);
            }
            // Close the socket to return its memory and bring down the network interface
            socket.close();
            failCount = 0;
        } else {
            failCount++;
            if (failCount == 4) {
                for(int j = 0; j < 2; j++) {
                    const int r = modem.connect(CELL_APN, CELL_USER, CELL_PWD);
                    if(r == NSAPI_ERROR_OK) break;
                }
                return;
            }
        }
        Thread::wait(60 * 1000);
    }
}

int main(void) {
    printf("Motion Detect v1.0\r\n");

    // connect modem
    const int r = modem.connect(CELL_APN, CELL_USER, CELL_PWD);
    // make sure we actually connected
    if (r == NSAPI_ERROR_OK) {
        printf("MODEM CONNECTED\r\n");
        // start sender thread
        sendData();
        // just loop around
        while (1) {
            led = !led;
            wait(1.0);
        }
    } else {
        printf("MODEM CONNECT FAILED\r\n");
        // just loop around
        while (1) {
            led = !led;
            wait(0.15);
        }
    }

}
#endif