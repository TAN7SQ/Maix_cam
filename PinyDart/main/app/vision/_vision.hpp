#pragma once

#include "_basic.hpp"

#include "_frameQueue.hpp"

class Vision
{
public:
    static constexpr const char *TAG = "Vision";
    void visionSchedule(void);
    Vision() : cameraFps(), visonFps() {};

    ~Vision();

private:
    /* Camera */
    // maix::camera::Camera *cam;
    // maix::camera::Camera *camRecord = nullptr;

    /* Threads */
    std::thread *pCameraThread = nullptr;
    std::thread *pVisionThread = nullptr;
    std::thread *pRecoderThread = nullptr;
    void cameraThread(maix::camera::Camera *pcam);
    void visionThread();
    void recoderThread();

    // 引导灯实际尺寸
    const float TARGET_SIZE = 5.0f; // cm
    // 摄像头内参
    const float FOCAL_LENGTH = 1000.0f; // 假设焦距为1000像素
    const int CAM_FPS = 80;
    const int IMG_WIDTH = 360;
    const int IMG_HEIGHT = 360;

    FPSCount cameraFps;
    FPSCount visonFps;
    FrameQueue frameQueue;
    FrameQueue recordQueue;
};
