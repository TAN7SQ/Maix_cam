#pragma once

#include "_basic.hpp"

#include "maix_pinmap.hpp"
#include "maix_uart.hpp"

using namespace maix;

#pragma pack(push, 1)
struct FrameHeader
{
    uint8_t sof;
    uint8_t version;
    uint8_t type;
    uint8_t flags;
    uint16_t seq;
    uint16_t len;
};
#pragma pack(pop)

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
    char *TAG = "uart";

    Uart(UartPort _port, int _baud) : port(_port), baud(_baud)
    {
    }
    ~Uart();

    void uartSchedule();

    int read(uint8_t *buf, int len);
    int write(const uint8_t *buf, int len);
    void run();
    void protocol_parse(uint8_t *buf, int len);

private:
    UartPort port;
    long baud;
    peripheral::uart::UART *serial = nullptr;
    std::thread *pUartThread = nullptr;

private:
    peripheral::uart::UART *uartInit(void);

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
