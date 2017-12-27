//
// Created by nirao on 15.12.17.
//

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"

#include <M66Interface.h>
#include "config.h"

using namespace utest::v1;

M66Interface modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER);

void TestfireUpModem(){
    TEST_ASSERT_TRUE_MESSAGE(modem.powerUpModem(), "modem power-up failed");
}

void TestresetModem(){
    TEST_ASSERT_TRUE_MESSAGE(modem.reset(), "modem reset failed");
}

void TestpowerDown(){
    TEST_ASSERT_TRUE_MESSAGE(modem.powerDown(), "modem power-down failed");
}

void TESTGetUnixTime(){

    TEST_ASSERT_EQUAL_MESSAGE(NSAPI_ERROR_OK, modem.connect(CELL_APN, CELL_USER, CELL_PWD), "modem connect failed");

    time_t ts;
    bool ret = modem.getUnixTime(&ts);

    printf("TS: %lu\r\n", ts);

    TEST_ASSERT_TRUE_MESSAGE(modem.getUnixTime(&ts), "Failed to get unix time");
}

utest::v1::status_t case_teardown_handler(const Case *const source, const size_t passed, const size_t failed,
                                          const failure_t reason) {
    printf("Close connection\r\n");
    modem.disconnect();
    return greentea_case_teardown_handler(source, passed, failed, reason);
}

utest::v1::status_t greentea_failure_handler(const Case *const source, const failure_t reason) {
    greentea_case_failure_abort_handler(source, reason);
    return STATUS_CONTINUE;
}

utest::v1::status_t greentea_test_setup(const size_t number_of_cases) {
    GREENTEA_SETUP(100, "default_auto");
    return verbose_test_setup_handler(number_of_cases);
}

int main() {

    Case cases[] = {
            Case("Test Unix Time Stamp", TESTGetUnixTime,
                 case_teardown_handler, greentea_failure_handler),
            Case("Test Unix Time Stamp1", TESTGetUnixTime,
                 case_teardown_handler, greentea_failure_handler),
            Case("Test Unix Time Stamp2", TESTGetUnixTime,
                 case_teardown_handler, greentea_failure_handler)
    };

    Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);
    return !Harness::run(specification);
}