
#include "maix_basic.hpp"
#include "maix_uart.hpp"
#include "main.h"

using namespace maix;
using namespace maix::peripheral;

int _main(int argc, char *argv[])
{
    int rx_len = 0;
    uint8_t rxBuff[128] = {0};

    std::vector<std::string> ports = uart::list_devices();
    for (auto &port : ports)
    {
        log::info("find uart port: %s\n", port.c_str());
    }
    /*
    # ./test04_nn_01
    -- [I] find uart port: /dev/ttyS0
    -- [I] find uart port: /dev/ttyS1
    -- [I] find uart port: /dev/ttyS2
    -- [I] find uart port: /dev/ttyS3
    -- [I] find uart port: /dev/serial0
    */

    log::info("open %s\n", ports[1].c_str());

    uart::UART serial1 = uart::UART(ports[1], 115200);

    std::string msg = "hello\r\n";
    serial1.write(msg.c_str());
    log::info("sent %s\n", msg.c_str());

    while (!app::need_exit())
    {
        rx_len = serial1.read(rxBuff, sizeof(rxBuff));
        if (rx_len > 0)
        {
            log::info("received %d: \"%s\"\n", rx_len, rxBuff);
            log::print(log::LogLevel::LEVEL_INFO, "hex:\n");
            for (int i = 0; i < rx_len; ++i)
            {
                log::print(log::LogLevel::LEVEL_INFO, "%02x ", rxBuff[i]);
            }
            log::print(log::LogLevel::LEVEL_INFO, "\n\n");
            serial1.write(rxBuff, rx_len);
            log::info("sent %d: \"%s\"\n", rx_len, rxBuff);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // Catch signal and process
    sys::register_default_signal_handle();

    // Use CATCH_EXCEPTION_RUN_RETURN to catch exception,
    // if we don't catch exception, when program throw exception, the objects will not be destructed.
    // So we catch exception here to let resources be released(call objects' destructor) before exit.
    CATCH_EXCEPTION_RUN_RETURN(_main, -1, argc, argv);
}
