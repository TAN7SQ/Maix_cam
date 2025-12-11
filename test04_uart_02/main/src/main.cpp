
#include "maix_basic.hpp"
#include "main.h"
#include "maix_uart.hpp"

#include "maix_pinmap.hpp"
#include <thread>

using namespace maix;

int _main(int argc, char *argv[])
{
    uint64_t t = time::time_s();
    log::info("Program start");

    int rx_len = 0;
    uint8_t rxBuff[128] = {0};

    const auto port = "/dev/ttyS1"s;       // UART port string
    constexpr long uart_baudrate = 115200; // UART baudrate constant

    auto set_uart_pin = [](const std::string &pin, const std::string &function)
    {
        auto ret = peripheral::pinmap::set_pin_function(pin.c_str(), function.c_str());
        if (ret != err::Err::ERR_NONE)
        {
            maix::log::error("pinmap error");
            return false;
        }
        return true;
    };
    set_uart_pin("A19", "UART1_TX");
    set_uart_pin("A18", "UART1_RX");
    maix::peripheral::uart::UART serial =
        maix::peripheral::uart::UART(
            std::string(port), static_cast<int>(uart_baudrate));

    std::string msg = "hello\r\n";
    serial.write(msg.c_str());
    log::info("sent %s", msg.c_str());
    // Run until app want to exit, for example app::switch_app API will set exit flag.
    // And you can also call app::set_exit_flag(true) to mark exit.
    while (!app::need_exit())
    {
        serial.write(msg.c_str());
        log::info("sent %s", msg.c_str());
        time::sleep_ms(100);

        rx_len = serial.read(rxBuff, sizeof(rxBuff));
        if (rx_len > 0)
        {
            log::info("received %d: \"%s\"", rx_len, rxBuff);
            log::print(log::LogLevel::LEVEL_INFO, "hex:\n");
            for (int i = 0; i < rx_len; ++i)
            {
                log::print(log::LogLevel::LEVEL_INFO, "%02x ", rxBuff[i]);
            }
            serial.write(rxBuff, rx_len);
            log::print(log::LogLevel::LEVEL_INFO, "\n\n");
        }
    }
    log::info("Program exit");

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
