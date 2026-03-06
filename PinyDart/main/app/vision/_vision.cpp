#include "_vision.hpp"

#include "_easyLog.hpp"

using namespace maix;

void Vision::visionSchedule(void)
{
    if (pVisionThread == nullptr) {
        pVisionThread = new std::thread(&Vision::visionThread, this);
    }
}

void Vision::visionThread(void)
{
    log::info("vision thread start");

    Log::info(TAG, "vision thread start");

    display::Display disp = display::Display();
    camera::Camera cam = camera::Camera(IMG_WIDTH, IMG_HEIGHT, image::FMT_BGR888);
    err::Err err;

    while (!app::need_exit()) {
        image::Image *img = cam.read();
        if (!img) {
            log::warn("Camera read image failed!");
            delete img;
            continue;
        }

        // TODO：视觉识别逻辑

        disp.show(*img);
        delete img;
        maix::thread::sleep_ms(10);
    }

    log::info("vision thread exit");
}

Vision::~Vision()
{
    log::info("vision thread destroy");
    if (pVisionThread) {
        if (pVisionThread->joinable()) {
            pVisionThread->join();
        }
        delete pVisionThread;
        pVisionThread = nullptr;
    }
}