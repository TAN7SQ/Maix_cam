#include "maix_basic.hpp"
#include "main.h"
#include "maix_uart.hpp"
#include "maix_pinmap.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <thread>
#include <mutex>

using namespace maix;

// ===================== 线程安全锁（避免数据竞争） =====================
std::mutex g_uart_mutex;    // 保护串口数据
std::mutex g_imu_mutex;     // 保护IMU数据
std::mutex g_vision_mutex;  // 保护视觉数据
std::mutex g_pos_vel_mutex; // 保护位置速度数据
// std::condition_variable g_imu_cv;    // IMU数据就绪通知
// std::condition_variable g_vision_cv; // 视觉数据就绪通知

int set_thread_priority(std::thread &thread, int policy, int priority)
{
    if (!thread.joinable())
    {
        log::error("thread not joinable");
        return -1;
    }
    pthread_t tid = thread.native_handle(); // 获取线程的原生句柄

    // 1. 设置调度策略
    struct sched_param param;
    memset(&param, 0, sizeof(param));
    param.sched_priority = priority;

    // 2. 设置调度策略和优先级
    if (pthread_setschedparam(tid, policy, &param) != 0)
    {
        log::error("pthread_setschedparam failed");
        return -1;
    }

    // 3. 获取当前线程的调度策略和优先级，验证设置是否成功
    struct sched_param current_param;
    if (pthread_getschedparam(tid, &policy, &current_param) != 0)
    {
        log::error("pthread_getschedparam failed");
        return -1;
    }
    log::info("thread policy: %d, priority: %d", policy, current_param.sched_priority);

    return 0;
}

void epoll_thread_func(void)
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

                        // 线程安全，写共享数据加锁
                        std::lock_guard<std::mutex> lock(g_uart_mutex);
                        // TODO: 添加队列

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

// TODO:新增线程2：视觉处理线程 =====================
void vision_thread_func()
{
}

// TODO:新增线程3：位置/速度解算线程 =====================
void calc_thread_func()
{
}

// 在这个线程里面就专门用来跑 epoll_wait，然后根据事件类型调用相应的处理函数
int _main(int argc, char *argv[])
{
    log::info("Program start");

    std::thread epoll_thread_fd(epoll_thread_func); // 构造函数，后面跟着参数
    std::thread vision_thread_fd(vision_thread_func);
    std::thread calc_thread_fd(calc_thread_func);

    set_thread_priority(vision_thread_fd, SCHED_FIFO, 90); // 视觉最高优先级
    set_thread_priority(calc_thread_fd, SCHED_FIFO, 80);
    set_thread_priority(epoll_thread_fd, SCHED_FIFO, 80);

    log::info("epoll_thread_func start, press Ctrl+C to exit\r\n");

    while (!app::need_exit())
    {
    }

    epoll_thread_fd.join(); // join会阻塞在这里,等待这个线程执行完成后才会返回
    // detach之后，主线程将不会对该线程进行管理
    //  vision_thread_fd.join();
    //  calc_thread_fd.join();

    log::info("Program exit");
    return 0;
}

int main(int argc, char *argv[])
{
    sys::register_default_signal_handle();
    CATCH_EXCEPTION_RUN_RETURN(_main, -1, argc, argv);
}
