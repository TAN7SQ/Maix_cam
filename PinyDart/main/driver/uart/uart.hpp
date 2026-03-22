#pragma once

#include "_basic.hpp"

#include "maix_pinmap.hpp"
#include "maix_uart.hpp"

using namespace maix;

#include "uart_parse.hpp"

enum class MsgType : uint8_t
{
    IMU = 1,
    BARO = 2,
    ATTITUDE = 3,
    CONTROL = 10,
    _SYSTEM = 20,
    _DEBUG = 100,
};

class Uart
{
public:
    enum UartPort
    {
        UART1 = 1,
        UART2 = 2,
    };

public:
    const char *TAG = "uart";

    Uart(UartPort _port, int _baud) : port(_port), baud(_baud)
    {
    }
    ~Uart();

    void uartSchedule();

    void run();

    void deinit(void);

private:
    UartPort port;
    long baud;
    peripheral::uart::UART *serial = nullptr;
    std::thread *pUartThread = nullptr;

    IMURawData rawData;
    BaroData baroData;
    IMUAttitude attitudeData;

private:
    peripheral::uart::UART *uartInit(void);

    int read(uint8_t *buf, int len);
    int write(const uint8_t *buf, int len);
    void parseProtocol(uint8_t *buf, int len);
    int sendProtocol(MsgType type, const void *payload, uint16_t length);

    bool set_uart_pin(const std::string &pin, const std::string &function)
    {
        auto ret = peripheral::pinmap::set_pin_function(pin.c_str(), function.c_str());
        if (ret != err::Err::ERR_NONE) {
            maix::log::error("pinmap error");
            return false;
        }
        return true;
    };
};