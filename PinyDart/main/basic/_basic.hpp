#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>

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

#include "_shared.hpp"
//================================================================

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

struct Quat
{
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f;
    Quat() = default;
    Quat(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_)
    {
    }
};

struct Mat3
{
    float m[3][3];
};

struct IMURawData
{
    Vec3 gyro;
    Vec3 acc;
};

struct IMUAttitude
{
    Vec3 euler;
    Quat quat;
};

struct BaroData
{
    double temperature = 0.0;   // 温度(℃)
    double pressure_mbar = 0.0; // 压力(mbar)
    double pressure_pa = 0.0;   // 压力(Pa)
    double height = 0.0;        // 高度(m)
};

//================================================================
struct CamTargetData
{
    bool valid = false;
    float area = 0.0f;

    float normX = 0.0f; // 去畸变后的归一化坐标 (用于制导)
    float normY = 0.0f;
    float rawCx = 0.0f; // 原始像素坐标 (用于画图调试)
    float rawCy = 0.0f;

    float yawCam = 0.0f;
    float pitchCam = 0.0f;
};

//================================================================
namespace Shared
{
extern std::atomic<bool> threadRun;
extern SharedQueue<CamTargetData> gTargetQueue;
extern SharedQueue<IMUAttitude> gImuAttitude;

}; // namespace Shared

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
        snprintf(fps_str, sizeof(fps_str), "%.1f", fps);
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

    static Quat quat_conjugate(const Quat &q)
    {
        return Quat(q.w, -q.x, -q.y, -q.z);
    }

    static Vec3 quat_rotate(const Quat &q, const Vec3 &v)
    {

        float qw = q.w, qx = q.x, qy = q.y, qz = q.z;

        Vec3 t;
        t.x = 2 * (qy * v.z - qz * v.y);
        t.y = 2 * (qz * v.x - qx * v.z);
        t.z = 2 * (qx * v.y - qy * v.x);

        Vec3 v_out;
        v_out.x = v.x + qw * t.x + (qy * t.z - qz * t.y);
        v_out.y = v.y + qw * t.y + (qz * t.x - qx * t.z);
        v_out.z = v.z + qw * t.z + (qx * t.y - qy * t.x);

        return v_out;
    }
};
