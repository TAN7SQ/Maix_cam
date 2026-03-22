#pragma once

#include "_basic.hpp"

#include "_configJson.hpp"
#include "_frameQueue.hpp"

#include "lpf.hpp"

#include "_shared.hpp"

#include <array>

// 使用半在线标定
namespace CalibParams
{
// 内参
constexpr float FX = 700.47896124f;
constexpr float FY = 699.73352985f;
constexpr float CX = 139.27813504f;
constexpr float CY = 163.85906361f;

// 畸变系数 [k1, k2, p1, p2, k3]
constexpr std::array<float, 5> DIST = {
    -3.63780738e-01f, 8.18599472e-01f, 9.43092466e-04f, 2.80046697e-03f, -5.16386081e+00f};
}; // namespace CalibParams

class Vision
{
public:
    struct SubpixelResult
    {
        float cx;
        float cy;
        float brightness;
    };

    typedef struct
    {
        int x;
        int y;
        int w;
        int h;

        float cx;
        float cy;
        float vx;
        float vy;

        int pixels;
        float brightness;
    } maxBlob_t;

    /********************************** */
public:
    static constexpr const char *TAG = "Vision";

    void visionSchedule(const VisionConfig &config);

    Vision() : cameraFps(), visonFps(), cxLpf(0.05f), cyLpf(0.05f) {};
    ~Vision();

    void deThread();

    /********************************** */
private:
    /* Threads */
    std::thread *pCameraThread = nullptr;
    std::thread *pVisionThread = nullptr;
    std::thread *pRecoderThread = nullptr;
    void cameraThread(maix::camera::Camera *pcam);
    void visionThread();
    void recoderThread();
    void recoderThread_just_record_mp4(void);

    void targetDetect(std::shared_ptr<maix::image::Image> img);
    void debugInfo(std::shared_ptr<maix::image::Image> img);

    /*--------------------------------------------------------------------- */
    float calcBlobBrightness(maix::image::Image *img, maix::image::Blob &blob);
    float calcBlobCenterBrightness(maix::image::Image *img, maix::image::Blob &blob);
    SubpixelResult calcBlobSubpixelCenter(maix::image::Image *img, maix::image::Blob &blob);

    // 去畸变
    void undistortPoint(float u, float v, float &x_out, float &y_out);

    /********************************** */
private:
    // 引导灯实际尺寸
    const float TARGET_SIZE = 5.0f; // cm
    // 摄像头内参
    const float FOCAL_LENGTH = 1000.0f; // 假设焦距为1000像素
    const int CAM_FPS = 80;
    const int IMG_WIDTH = 320;
    const int IMG_HEIGHT = 320;

    FPSCount cameraFps;
    FPSCount visonFps;
    FrameQueue frameQueue;
    FrameQueue recordQueue;

    /********************************** */
    maxBlob_t maxblob;
    CamTargetData target;

    VisionConfig _config; // json配置文件数据

    int sock;
    struct sockaddr_in addr;

    /********************************** */
    LPF cxLpf;
    LPF cyLpf;
};
