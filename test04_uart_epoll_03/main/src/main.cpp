#include "maix_basic.hpp"
#include "main.h"
#include "maix_uart.hpp"
#include "maix_pinmap.hpp"

#include <sys/epoll.h>
#include <unistd.h>
#include <thread>

using namespace maix;

void epoll_thread_func(int none)
{
    log::info("epoll start");

    uint8_t rxBuff[128] = {0};
    const auto port = "/dev/ttyS1"s;
    constexpr long uart_baudrate = 1500000;

    auto set_uart_pin = [](const std::string &pin, const std::string &function)
    {
        auto ret = maix::peripheral::pinmap::set_pin_function(pin.c_str(), function.c_str());
        if (ret != err::Err::ERR_NONE)
        {
            maix::log::error("pinmap error");
            return false;
        }
        return true;
    };
    set_uart_pin("A19", "UART1_TX");
    set_uart_pin("A18", "UART1_RX");

    maix::peripheral::uart::UART serial(
        std::string(port), static_cast<int>(uart_baudrate));

    int fd = serial.get_fd();
    // 创建 epoll
    int epfd = epoll_create1(0);
    if (epfd == -1)
    {
        log::error("epoll_create1 failed");
    }

    struct epoll_event ev;
    ev.events = EPOLLIN; // 监听可读
    // ev.events = EPOLLIN | EPOLLOUT; // 监听可读和可写
    ev.data.fd = fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        log::error("epoll_ctl failed");
    }

    std::string msg = "hello\r\n";

    while (!app::need_exit())
    {
        struct epoll_event events[10];
        int nfds = epoll_wait(epfd, events, 10, 1000); // 1秒超时
        if (nfds == 0)
        {
            // 超时无事件
            continue;
        }
        if (nfds == -1)
        {
            log::error("epoll_wait error");
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == fd)
            {
                if (events[i].events & EPOLLIN)
                {
                    int rx_len = serial.read(rxBuff, sizeof(rxBuff));
                    if (rx_len > 0)
                    {
                        log::info("received %d: \"%s\"", rx_len, rxBuff);
                        log::print(log::LogLevel::LEVEL_INFO, "hex:\n");
                        for (int j = 0; j < rx_len; ++j)
                        {
                            log::print(log::LogLevel::LEVEL_INFO, "%02x ", rxBuff[j]);
                        }
                        log::print(log::LogLevel::LEVEL_INFO, "\n\n");
                        // 回写
                        serial.write(rxBuff, rx_len);
                        // 填入串口接收队列
                    }
                }

                if (events[i].events & EPOLLOUT)
                {
                    // 可写时发送数据
                    serial.write(msg.c_str());
                    log::info("sent %s", msg.c_str());
                }
            }
        }
    }
    log::info("epoll exit");
    close(epfd);
}

// 在这个线程里面就专门用来跑 epoll_wait，然后根据事件类型调用相应的处理函数
int _main(int argc, char *argv[])
{
    log::info("Program start");

    std::thread epoll_thread_fd(epoll_thread_func, 0); // 创建并启动线程
    epoll_thread_fd.join();
    log::info("epoll_thread_func start");

    while (!app::need_exit())
    {
    }

    log::info("Program exit");
    return 0;
}

int main(int argc, char *argv[])
{
    sys::register_default_signal_handle();
    CATCH_EXCEPTION_RUN_RETURN(_main, -1, argc, argv);
}
