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

        this->set_uart_pin("A19", "UART1_TX");
        this->set_uart_pin("A18", "UART1_RX");

        static peripheral::uart::UART serial = peripheral::uart::UART(std::string(fdport), this->baud);

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
    while (Shared::threadRun) {
        maix::thread::sleep_ms(1);
        if (serial->available() > 0) {
            int n = serial->read(buf, sizeof(buf));
            if (n <= 0) {
                continue;
            }

            parseProtocol(buf, n);
        }
    }
    // serial->close();
}

int Uart::read(uint8_t *buf, int len)
{
    if (!buf || len <= 0)
        return -1;

    return this->serial->read(buf, len);
}

int Uart::write(const uint8_t *buf, int len)
{
    if (!buf || len <= 0)
        return -1;

    return this->serial->write(buf, len);
}

void Uart::parseProtocol(uint8_t *buf, int len)
{
    static UartFrameParser parser;
    static Frame frame;
    for (int i = 0; i < len; i++) {
        if (parser.input(buf[i], frame)) {
            switch (frame.type) {
            case (uint8_t)MsgType::IMU: {
                memcpy(&rawData, frame.payload, sizeof(IMURawData));
                break;
            }
            case (uint8_t)MsgType::BARO: {
                memcpy(&baroData, frame.payload, sizeof(BaroData));
                break;
            }
            case (uint8_t)MsgType::ATTITUDE: {
                memcpy(&attitudeData, frame.payload, sizeof(IMUAttitude));
                Shared::gImuAttitude.push_latest(attitudeData);
                break;
            }
            case (uint8_t)MsgType::CONTROL:
                Log::info(TAG, "control");
                break;
            default:
                Log::error(TAG, "unknown type:%d", frame.type);
                break;
            }
        }
    }
}

int Uart::sendProtocol(MsgType type, const void *payload, uint16_t payloadLength)
{
    if (!payload || payloadLength <= 0)
        return -1;

    const uint8_t SOF = 0xAA;

    // AA  | TYPE | LEN | PAYLOAD | CRC16
    uint16_t frameLength = 1 + 1 + 2 + payloadLength + 2;

    if (frameLength > MAX_PAYLOAD)
        return -1;
    uint16_t index = 0;

    uint8_t frame[frameLength];
    frame[index++] = SOF;
    frame[index++] = (uint8_t)type;
    frame[index++] = payloadLength & 0xFF;
    frame[index++] = (payloadLength >> 8) & 0xFF;
    memcpy(frame + index, payload, payloadLength);
    index += payloadLength;

    uint16_t crc = Tools::crc16_ccitt(frame + 1, index - 2);
    frame[index++] = crc & 0xFF;
    frame[index++] = (crc >> 8) & 0xFF;

    return this->write(frame, frameLength);
}

Uart::~Uart()
{
}

void Uart::deinit()
{
    if (this->pUartThread != nullptr) {
        this->pUartThread->join();
        delete this->pUartThread;
        this->pUartThread = nullptr;
    }
    if (this->serial) {
        this->serial->close();
    }
    Log::info(TAG, "deinit");
}