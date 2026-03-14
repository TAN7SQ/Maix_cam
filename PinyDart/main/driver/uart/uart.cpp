#include "uart.hpp"
#include "_basic.hpp"

static constexpr uint8_t SOF = 0xAA;
static constexpr uint16_t MAX_PAYLOAD = 256;

/* ---------------- 状态机 ---------------- */

enum class ParseState
{
    WAIT_SOF,
    READ_HEADER,
    READ_PAYLOAD,
    READ_CRC
};

peripheral::uart::UART *Uart::uartInit(void)
{
    if (this->port == UART1) {
        this->TAG = (char *)"uart1";
        Log::info(TAG, "port:%d,baud:%d", this->port, this->baud);
        auto fdport = "/dev/ttyS1"s;
        int uart_baudrate = 115200;

        this->set_uart_pin("A19", "UART1_TX");
        this->set_uart_pin("A18", "UART1_RX");

        static peripheral::uart::UART serial = peripheral::uart::UART(std::string(fdport), uart_baudrate);

        Log::info(TAG, "init uart1 success");
        return &serial;
    }
    else if (this->port == UART2) {
        this->TAG = (char *)"uart2";
        Log::error(this->TAG, "not supported");
        return nullptr;
    }

    return nullptr;
}

void Uart::uartSchedule()
{
    if (this->pUartThread == nullptr) {
        this->pUartThread = new std::thread(&Uart::run, this);
        pthread_setname_np(this->pUartThread->native_handle(), "uart_thread");
    }
}

void Uart::run()
{
    serial = uartInit();
    if (!serial) {
        Log::error(TAG, "uartInit failed");
        return;
    }
    uint8_t buf[256];
    while (!app::need_exit()) {
        maix::thread::sleep_ms(1);
        if (serial->available() > 0) {
            int n = serial->read(buf, sizeof(buf));
            if (n > 0) {
                printf("uart read %d bytes: ", n);
                for (int i = 0; i < n; i++) {
                    printf("%02X ", buf[i]);
                }
                printf("\n");
                protocol_parse(buf, n);
            }
        }
    }
}

// int Uart::read(uint8_t *buf, int len)
// {
//     if (!buf || len <= 0)
//         return -1;

//     return this->serial.read(buf, len, -1, 0);
// }

// int Uart::write(const uint8_t *buf, int len)
// {
//     if (!buf || len <= 0)
//         return -1;

//     return this->serial.write(buf, len);
// }

void Uart::protocol_parse(uint8_t *buf, int len)
{
    static ParseState state = ParseState::WAIT_SOF;

    static uint8_t headerBuf[sizeof(FrameHeader)];
    static uint16_t headerIndex = 0;

    static FrameHeader header;

    static uint8_t payload[MAX_PAYLOAD];
    static uint16_t payloadIndex = 0;

    static uint8_t crcBuf[2];
    static uint8_t crcIndex = 0;

    static uint16_t lastSeq = 0;

    for (int i = 0; i < len; i++) {

        uint8_t byte = buf[i];

        switch (state) {
        case ParseState::WAIT_SOF:
            if (byte == SOF) {
                headerBuf[0] = byte;
                headerIndex = 1;
                state = ParseState::READ_HEADER;
            }
            break;
        case ParseState::READ_HEADER:
            headerBuf[headerIndex++] = byte;
            if (headerIndex == sizeof(FrameHeader)) {
                memcpy(&header, headerBuf, sizeof(FrameHeader));

                if (header.len > MAX_PAYLOAD) {
                    state = ParseState::WAIT_SOF;
                    break;
                }
                payloadIndex = 0;
                state = ParseState::READ_PAYLOAD;
            }
            break;
        case ParseState::READ_PAYLOAD:

            payload[payloadIndex++] = byte;

            if (payloadIndex == header.len) {

                crcIndex = 0;
                state = ParseState::READ_CRC;
            }

            break;
        case ParseState::READ_CRC:

            crcBuf[crcIndex++] = byte;

            if (crcIndex == 2) {
                uint16_t recvCrc = crcBuf[0] | (crcBuf[1] << 8);
                uint16_t calcCrc = Tools::crc16_ccitt(headerBuf, sizeof(FrameHeader));
                calcCrc = Tools::crc16_ccitt(payload, header.len, calcCrc);
                if (recvCrc == calcCrc) {

                    if (header.seq != lastSeq + 1 && lastSeq != 0) {
                        Log::warn(this->TAG, "packet lost seq=%d last=%d", header.seq, lastSeq);
                    }

                    lastSeq = header.seq;

                    switch (header.type) {

                    case (uint8_t)MsgType::IMU:
                        if (header.len == sizeof(IMUAttitude)) {
                            IMUAttitude att;

                            memcpy(&att, payload, sizeof(att));

                            printf("roll  = %f\n", att.euler.roll);
                            printf("pitch = %f\n", att.euler.pitch);
                            printf("yaw   = %f\n", att.euler.yaw);
                        }
                        break;

                    case (uint8_t)MsgType::BARO:
                        break;

                    case (uint8_t)MsgType::ATTITUDE:
                        break;

                    case (uint8_t)MsgType::CONTROL:
                        break;

                    default:
                        break;
                    }
                }
                else {
                    Log::warn(this->TAG, "crc error");
                }
                state = ParseState::WAIT_SOF;
            }
            break;
        }
    }
}

Uart::~Uart()
{
    if (this->pUartThread != nullptr) {
        this->pUartThread->join();
        delete this->pUartThread;
        this->pUartThread = nullptr;
    }
}