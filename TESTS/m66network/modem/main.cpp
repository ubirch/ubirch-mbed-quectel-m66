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

M66Interface modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER);


void fireUpModem() {
    TEST_ASSERT_TRUE_MESSAGE(modem.powerUpModem(), "modem power-up failed");
}

void checkModem() {
    TEST_ASSERT_TRUE_MESSAGE(modem.isModemAlive(), "modem alive check failed");
}

void resetModem() {
    TEST_ASSERT_TRUE_MESSAGE(modem.reset(), "modem reset failed");
}

void powerDown() {
    TEST_ASSERT_TRUE_MESSAGE(modem.powerDown(), "modem power-down failed");
}

void modemIMEI() {
    TEST_ASSERT_EQUAL(NSAPI_ERROR_OK, modem.set_imei());
}

void modemICCID() {
    TEST_ASSERT_NOT_NULL(modem.get_iccid());
}

#if defined(CELL_APN) && defined(CELL_USER) && defined(CELL_PWD)

void modemConnect() {
    TEST_ASSERT_TRUE_MESSAGE(modem.powerUpModem(), "modem power-up failed");
    TEST_ASSERT_EQUAL_MESSAGE(NSAPI_ERROR_OK, modem.connect(NULL, CELL_APN, CELL_USER, CELL_PWD),
                              "modem connect failed");
}

void modemHTTP() {

    int ret;
    TCPSocket socket;

    ret = socket.open(&modem);
    TEST_ASSERT_EQUAL_MESSAGE(NSAPI_ERROR_OK, ret, "socket open failed");

    ret = socket.connect("www.arm.com", 80);
    TEST_ASSERT_EQUAL_MESSAGE(NSAPI_ERROR_OK, ret, "socket connect failed");

    char theUrl[] = "GET / HTTP/1.1\r\n\r\n";
    int sendCount = socket.send(theUrl, sizeof(theUrl));
    TEST_ASSERT_EQUAL_MESSAGE(sendCount, sizeof(theUrl), "socket send failed");

    // receive a simple http response and check if it's not empty
    char rbuffer[64];
    int rcount = socket.recv(rbuffer, sizeof rbuffer);
    TEST_ASSERT_TRUE_MESSAGE(rcount > 0, "socket recv failed");

    ret = socket.close();
    TEST_ASSERT_EQUAL_MESSAGE(NSAPI_ERROR_OK, ret, "socket close failed");

    ret = modem.disconnect();
    TEST_ASSERT_EQUAL_MESSAGE(NSAPI_ERROR_OK, ret, "modem disconnect failed ");
}

#endif

utest::v1::status_t greentea_failure_handler(const Case *const source, const failure_t reason) {
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

Case cases[] = {
    Case("Modem PowerUp-0", fireUpModem, greentea_failure_handler),
//    Case("Modem Alive-0", checkModem, greentea_failure_handler),
//    Case("Modem Reset-0", resetModem, greentea_failure_handler),
//    Case("Modem Alive-1", checkModem, greentea_failure_handler),
//    Case("Modem Reset-1", resetModem, greentea_failure_handler),
//    Case("Modem Reset-2", resetModem, greentea_failure_handler),
//    Case("Modem Reset-3", resetModem, greentea_failure_handler),
//    Case("Modem Reset-4", resetModem, greentea_failure_handler),
//    Case("Modem PowerDown", powerDown, greentea_failure_handler),
    Case("Modem get IMEI", modemIMEI, greentea_failure_handler),
    Case("Modem get ICCID", modemICCID, greentea_failure_handler),
#if defined(CELL_APN) && defined(CELL_USER) && defined(CELL_PWD)
    Case("Connect-0", modemConnect, greentea_failure_handler),
    Case("HTTP Connect-0", modemHTTP, greentea_failure_handler),
#else
#warning "CONNECTIONS NOT TESTED: set CELL_APN, CELL_USER, CELL_PWD in config.h"
#endif
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases) {
    GREENTEA_SETUP(150, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}


int main() {
    Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);
    Harness::run(specification);
}
