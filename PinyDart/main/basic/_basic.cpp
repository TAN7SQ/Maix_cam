#include "_basic.hpp"

#include "maix_basic.hpp"
#include "maix_pinmap.hpp"
#include "maix_uart.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <mutex>
#include <thread>

using namespace maix;

uint16_t Tools::crc16_ccitt(const uint8_t *data, int len)
{
    uint16_t crc = 0xFFFF;

    for (int i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }

    return crc;
}

uint16_t Tools::crc16_ccitt(const uint8_t *data, uint16_t len, uint16_t crc)
{
    for (int i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }

    return crc;
}
