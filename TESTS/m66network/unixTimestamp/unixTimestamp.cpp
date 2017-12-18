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

void TESTGetUnixTime(){
    /*char k[128], v[256];

    greentea_send_kv("unixTime", "hello");
    greentea_parse_kv(k, v, sizeof(k), sizeof(v));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("OK", v, "failed to receive OK");*/

    TEST_ASSERT_EQUAL_MESSAGE(NSAPI_ERROR_OK, modem.connect(CELL_APN, CELL_USER, CELL_PWD), "modem connect failed");

    char ip[20] = {0};
    modem.queryIP("www.arm.com", &ip[0]);
    time_t ts;
    bool ret = modem.getUnixTime(&ts);
    printf("TS: %lu\r\n", ts);
    TEST_ASSERT_UNLESS_MESSAGE(ts == 0, "Failed to get unix time");
}

void TESTGetTime(){

    TEST_ASSERT_EQUAL_MESSAGE(NSAPI_ERROR_OK, modem.connect(CELL_APN, CELL_USER, CELL_PWD), "modem connect failed");

    time_t ts;
    bool ret = modem.getUnixTime(&ts);
    printf("TS: %lu\r\n", ts);
    TEST_ASSERT_UNLESS_MESSAGE(ts == 0, "Failed to get unix time");
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
                 case_teardown_handler, greentea_failure_handler),
            Case("Test TimeStamp", TESTGetTime,
                 case_teardown_handler, greentea_failure_handler),
            Case("Test TimeStamp1", TESTGetTime,
                 case_teardown_handler, greentea_failure_handler),
            Case("Test TimeStamp3", TESTGetTime,
                 case_teardown_handler, greentea_failure_handler)
    };

    Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);
    return !Harness::run(specification);
}