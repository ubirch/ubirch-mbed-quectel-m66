/* mbed Microcontroller Library
 * Copyright (c) 2016 ARM Limited
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

//#include "mbed.h"

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include "M66Interface.h"
#include "config.h"

using namespace utest::v1;

M66Interface modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER, true);

void fireUpModem(){
    int ret = modem.powerUpModem();
    TEST_ASSERT_UNLESS_MESSAGE(ret == 0, "Failed to power up the modem");
}
void connect_modem(){
    int ret;
    ret = modem.connect(CELL_APN, CELL_USER, CELL_PWD);
    TEST_ASSERT_UNLESS_MESSAGE(ret != 0, "Not Connected");
}

void modemHTTP() {

    int ret;
    TCPSocket socket;

    ret = socket.open(&modem);
    TEST_ASSERT_MESSAGE(ret == 0, "Open Socket");

    ret = socket.connect("www.arm.com", 80);
    printf("socket connect %d\r\n", ret);
    TEST_ASSERT_UNLESS_MESSAGE(ret != 0, "Connected fail");

    char theUrl[] = "GET /HTTP/1.1\r\n\r\n";
    int sendCount = socket.send(theUrl, sizeof(theUrl));
    printf("%d send count\r\n", sendCount);
    TEST_ASSERT_MESSAGE(sendCount > 0, "Socket send sucess");

    // Recieve a simple http response and check if it's not empty
    char rbuffer[64];
    int rcount = socket.recv(rbuffer, sizeof rbuffer);
    printf("%d receive count\r\n", rcount);
//    TEST_ASSERT_MESSAGE(rcount <= 0, "Socket recv error");
    TEST_ASSERT_MESSAGE(rcount > 0, "No data received");

    ret = socket.close();
    TEST_ASSERT_MESSAGE(ret == 0, "Socket Close Error");

    ret = modem.disconnect();
    TEST_ASSERT_MESSAGE(ret == 0, "Disconnect ");

    ret = modem.powerDown();
    TEST_ASSERT_UNLESS_MESSAGE(ret == 0, "Abnormal PowerDown");
}

void checkModem(){
    int ret;

    ret = modem.isModem();
    TEST_ASSERT_UNLESS_MESSAGE(ret == 0, "Modem is dead");
}

void resetModem(){
    int ret = modem.reset();
    TEST_ASSERT_UNLESS_MESSAGE(ret == 0, "reset failed");
}

void powerDown(){
    int ret = modem.powerDown();
    TEST_ASSERT_UNLESS_MESSAGE(ret ==  0, "Power Down fail!");
}
utest::v1::status_t greentea_failure_handler(const Case *const source, const failure_t reason) {
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

Case cases[] = {
        Case("Modem PowerUp-0", fireUpModem, greentea_failure_handler),
        Case("Modem Alive-0", checkModem, greentea_failure_handler),
        Case("Modem Reset-0", resetModem, greentea_failure_handler),
        Case("Connect-0", connect_modem, greentea_failure_handler),
//        Case("HTTP Connect-0", modemHTTP, greentea_failure_handler),
        Case("Modem PowerUp-1", fireUpModem, greentea_failure_handler),
        Case("Modem Alive-1", checkModem, greentea_failure_handler),
        Case("Modem Reset-1", resetModem, greentea_failure_handler),
        Case("Modem Reset-2", resetModem, greentea_failure_handler),
        Case("Modem Reset-3", resetModem, greentea_failure_handler),
        Case("Modem Reset-4", resetModem, greentea_failure_handler),
        Case("Modem PowerDown ", powerDown, greentea_failure_handler),
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases) {
    GREENTEA_SETUP(150, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}


int main() {
    Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);
    Harness::run(specification);
}
