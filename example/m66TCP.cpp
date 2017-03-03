/*
 * Example program establishes TCP/IP connection,
 * and sends a HTTP GET request
 */

#if 0
#include "mbed.h"
#include "../M66Interface.h"

#define STACK_SIZE 24000

DigitalOut led1(LED1);

M66Interface modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER, true);

void led_thread(void const *args) {
    while (true) {
        led1 = !led1;
        Thread::wait(1000);
    }
}

void http_get(M66Interface *net)
{
    TCPSocket socket;

    printf("Sending HTTP request to api.ubirch.com...\r\n");

    // Open a socket on the network interface, and create a TCP connection to www.arm.com
    socket.open(net);

    socket.connect("api.ubirch.com", 80);

    // Send a simple http request
    char sbuffer[] = "GET / HTTP/1.1\r\n\r\n";

    int scount = socket.send(sbuffer, sizeof sbuffer);
    printf("sent %d [%.*s]\r\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);

    // Recieve a simple http response and print out the response line
    char rbuffer[64];
    int rcount = socket.recv(rbuffer, sizeof rbuffer);
    printf("recv %d [%.*s]\r\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);

    // Close the socket to return its memory and bring down the network interface
    socket.close();
}

void modem_thread(void const *args) {

    printf("Quectel M66 Driver example\r\n\r\n");

    printf("\r\nConnecting...\r\n");
    int ret = modem.connect(CELL_APN, CELL_USER, CELL_PWD);
    if (ret != 0) {
        printf("\r\nConnection error\r\n");
    }
    else {
        printf("Connection Success\r\n\r\n");
        printf("IP: %s\r\n", modem.get_ip_address());

        http_get(&modem);

        modem.disconnect();

        printf("\r\nDone\r\n");
    }
}

osThreadDef(led_thread,   osPriorityNormal, DEFAULT_STACK_SIZE);
osThreadDef(modem_thread, osPriorityNormal, STACK_SIZE);

// main() runs in its own thread in the OS
// (note the calls to Thread::wait below for delays)
int main() {

    osThreadCreate(osThread(led_thread), NULL);
    osThreadCreate(osThread(modem_thread), NULL);

    while (true) {
        led1 = !led1;
        Thread::wait(200);
    }
}

#endif