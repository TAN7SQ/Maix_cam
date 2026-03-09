#pragma once

#include <cstdint>
#include <cstring>

//================================================================

#include "opencv2/freetype.hpp"
#include "opencv2/opencv.hpp"

#include "maix_basic.hpp"
#include "maix_camera.hpp"
#include "maix_display.hpp"
#include "maix_image_cv.hpp"
#include "maix_thread.hpp"
#include "maix_video.hpp"

#include <thread>

//================================================================
#include "_easyLog.hpp"

class FPSCount
{
public:
    FPSCount(float alpha = 0.1f) : alpha(alpha)
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
            fps = inst_fps;
            initialized = true;
        }
        else {
            fps = (1.0f - alpha) * fps + alpha * inst_fps;
        }
        snprintf(fps_str, sizeof(fps_str), "%.2f FPS", fps);
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