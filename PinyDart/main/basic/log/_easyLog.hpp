#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[1;31m"
#define ANSI_GREEN "\033[1;32m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_BLUE "\033[1;34m"
#define ANSI_PURPLE "\033[1;35m"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

class Log
{
public:
    enum LogLevel
    {
        LOG_LEVEL_ERROR = 0,
        LOG_LEVEL_WARN,
        LOG_LEVEL_INFO,
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_TRACE
    };

    static void init()
    {
        start_time = std::chrono::steady_clock::now();
        running = true;
        log_thread = std::thread(logThread);
    }
    static void enable_udp(const char *ip, int port)
    {
        udp_fd = socket(AF_INET, SOCK_DGRAM, 0);

        if (udp_fd < 0) {
            perror("udp socket failed");
            return;
        }
        fcntl(udp_fd, F_SETFL, O_NONBLOCK);
        udp_addr.sin_family = AF_INET;
        udp_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &udp_addr.sin_addr);

        udp_enable = true;
    }

    static void shutdown()
    {
        running = false;
        if (log_thread.joinable())
            log_thread.join();
        cv.notify_all();
        if (udp_fd > 0)
            close(udp_fd);
    }

    static void setLogLevel(LogLevel level)
    {
        current_level = level;
    }

    static LogLevel getLogLevel()
    {
        return current_level;
    }

    static void error(const char *tag, const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        push_log(LOG_LEVEL_ERROR, tag, fmt, args);
        va_end(args);
    }
    static void warn(const char *tag, const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        push_log(LOG_LEVEL_WARN, tag, fmt, args);
        va_end(args);
    }
    static void info(const char *tag, const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        push_log(LOG_LEVEL_INFO, tag, fmt, args);
        va_end(args);
    }
    static void debug(const char *tag, const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        push_log(LOG_LEVEL_DEBUG, tag, fmt, args);
        va_end(args);
    }
    static void trace(const char *tag, const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        push_log(LOG_LEVEL_TRACE, tag, fmt, args);
        va_end(args);
    }

private:
    static inline int udp_fd = -1;
    static inline sockaddr_in udp_addr;
    static inline bool udp_enable = false;
    /********************************************* */
    static inline std::chrono::steady_clock::time_point start_time;
    static inline LogLevel current_level = LOG_LEVEL_TRACE;

    static inline std::mutex mtx;
    static inline std::condition_variable cv;
    static inline std::queue<std::string> log_queue;
    static inline std::thread log_thread;
    static inline std::atomic<bool> running{false};

    static constexpr size_t MAX_QUEUE_SIZE = 5000;

    static uint32_t get_elapsed_ms()
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
    }

    static void push_log(LogLevel level, const char *tag, const char *fmt, va_list args)
    {
        if (level > current_level)
            return;

        char msg[512];
        vsnprintf(msg, sizeof(msg), fmt, args);

        const char *level_char = "";
        const char *color = ANSI_RESET;

        switch (level) {
        case LOG_LEVEL_ERROR:
            level_char = "E";
            color = ANSI_RED;
            break;
        case LOG_LEVEL_WARN:
            level_char = "W";
            color = ANSI_YELLOW;
            break;
        case LOG_LEVEL_INFO:
            level_char = "I";
            color = ANSI_GREEN;
            break;
        case LOG_LEVEL_DEBUG:
            level_char = "D";
            color = ANSI_BLUE;
            break;
        case LOG_LEVEL_TRACE:
            level_char = "T";
            color = ANSI_PURPLE;
            break;
        }

        char final_msg[700];
        snprintf(final_msg,
                 sizeof(final_msg),
                 "%s%s (%u) %s: %s%s",
                 color,
                 level_char,
                 get_elapsed_ms(),
                 tag,
                 msg,
                 ANSI_RESET);

        {
            std::lock_guard<std::mutex> lock(mtx);

            if (log_queue.size() >= MAX_QUEUE_SIZE) {
                log_queue.pop();
            }

            log_queue.push(final_msg);
        }

        cv.notify_one();
    }

    static void logThread()
    {
        while (running) {
            std::unique_lock<std::mutex> lock(mtx);

            cv.wait(lock, [] {
                return !log_queue.empty() || !running;
            });

            while (!log_queue.empty()) {
                std::string msg = std::move(log_queue.front());
                log_queue.pop();

                lock.unlock();
                printf("%s\n", msg.c_str());

                if (udp_enable) {
                    sendto(udp_fd, msg.c_str(), msg.size(), 0, (struct sockaddr *)&udp_addr, sizeof(udp_addr));
                }
                lock.lock();
            }
        }

        while (!log_queue.empty()) {
            printf("%s\n", log_queue.front().c_str());
            log_queue.pop();
        }
    }
};
