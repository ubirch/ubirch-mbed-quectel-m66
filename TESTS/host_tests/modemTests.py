from mbed_host_tests import BaseHostTest, event_callback


class UnixTimeTests(BaseHostTest):
    """
    Modem Tests

    Simple Tests to test if the modem gets the right UTC time

    """

    def __init__(self):
        BaseHostTest.__init__(self)

    @event_callback("unixTime")
    def __unixTime(self, key, value, timestamp):
        self.log("unix:" + key + "(" + value + ")")
        if value is "hello":
            self.send_kv("timestamp", 'OK')
        else:
            self.log("received nothing" + key)
            self.send_kv("timestamp", 'OK')