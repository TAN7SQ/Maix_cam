#pragma once

#include <chrono> // 用于计算启动时间
#include <cstdarg>
#include <cstdio>
#include <cstring>

#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[1;31m"
#define ANSI_GREEN "\033[1;32m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_BLUE "\033[1;34m"
#define ANSI_PURPLE "\033[1;35m"

typedef enum
{
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_ERROR = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_TRACE = 4
} LogLevel;

class Log
{
public:
    static void init();

    static void info(const char *TAG, const char *format, ...);  // Info
    static void warn(const char *TAG, const char *format, ...);  // Warn
    static void error(const char *TAG, const char *format, ...); // Error
    static void debug(const char *TAG, const char *format, ...); // Debug
    static void trace(const char *TAG, const char *format, ...); // Trace

private:
    static std::chrono::steady_clock::time_point start_time;
    static void log_print(LogLevel level, const char *TAG, const char *format, va_list args);
    static uint32_t get_elapsed_ms();
};
