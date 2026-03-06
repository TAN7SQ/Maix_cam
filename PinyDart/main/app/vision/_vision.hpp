#pragma once

#include "_basic.hpp"

class Vision
{
public:
    static constexpr const char *TAG = "Vision";
    void visionSchedule(void);
    Vision() : pVisionThread(nullptr) {};

    ~Vision();

private:
    std::thread *pVisionThread;
    void visionThread(void);

    // 引导灯实际尺寸
    const float TARGET_SIZE = 5.0f; // cm
    // 摄像头内参
    const float FOCAL_LENGTH = 1000.0f; // 假设焦距为1000像素
    const int IMG_WIDTH = 640;
    const int IMG_HEIGHT = 480;
};
