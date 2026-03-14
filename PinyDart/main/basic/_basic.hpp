#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

//================================================================

#include "opencv2/freetype.hpp"
#include "opencv2/opencv.hpp"

#include "maix_basic.hpp"
#include "maix_camera.hpp"
#include "maix_display.hpp"
#include "maix_err.hpp"
#include "maix_image_cv.hpp"
#include "maix_rtsp.hpp"
#include "maix_thread.hpp"
#include "maix_util.hpp"
#include "maix_video.hpp"

#include "rtsp_server.h"
#include "sophgo_middleware.hpp"

#include <thread>

//================================================================
#include "_easyLog.hpp"
struct Vec3
{
    union {
        struct
        {
            float x, y, z;
        };
        struct
        {
            float roll, pitch, yaw;
        };
        float v[3];
    };
    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_)
    {
    }
};

struct IMURawData
{
    Vec3 gyro;
    Vec3 acc;
};

struct IMUAttitude
{
    Vec3 euler;
    Vec3 quat;
};

//================================================================

class FPSCount
{
public:
    FPSCount(float alpha = 0.2f) : alpha(alpha)
    {
        last_time = maix::time::ticks_ms();
    }

    void tick()
    {
        uint64_t now = maix::time::ticks_ms();
        uint64_t delta = now - last_time;
        if (delta == 0)
            return;
        float inst_fps = 1000.0f / delta;
        if (!initialized) {
            fps = 30;
            initialized = true;
        }
        else {
            fps = (1.0f - alpha) * fps + alpha * inst_fps;
        }
        snprintf(fps_str, sizeof(fps_str), "%.2f", fps);
        last_time = now;
    }

    float get() const
    {
        return fps;
    }

    const char *str() const
    {
        return fps_str;
    }

private:
    uint64_t last_time = 0;
    float fps = 0.0f;
    float alpha;
    bool initialized = false;

    char fps_str[32] = {0};
};

#include <fstream>
#include <string>
class Temp
{
public:
    static std::string get_cpu_temp()
    {
        std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
        if (!file.is_open())
            return "error";

        int temp;
        file >> temp;

        char buf[64];
        sprintf(buf, "T:%.2f", temp / 1000.0f);

        return buf;
    }
};

class Tools
{
public:
    static constexpr const char *TAG = "Tools";
    static uint16_t crc16_ccitt(const uint8_t *data, int len);
    static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len, uint16_t crc);
};
