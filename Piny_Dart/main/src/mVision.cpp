#include "mVision.hpp"
#include "maix_basic.hpp"
#include "main.h"

#include "opencv2/opencv.hpp"
#include "opencv2/freetype.hpp"

#include "maix_display.hpp"
#include "maix_camera.hpp"
#include "maix_image_cv.hpp"

#include "maix_thread.hpp"

using namespace maix;

// 引导灯实际尺寸
const float TARGET_SIZE = 5.0f; // cm
// 摄像头内参
const float FOCAL_LENGTH = 1000.0f; // 假设焦距为1000像素
const int IMG_WIDTH = 640;
const int IMG_HEIGHT = 480;

namespace mVision
{
    void vision_thread(void)
    {
        log::info("vision thread start");

        display::Display disp = display::Display();
        camera::Camera cam = camera::Camera(IMG_WIDTH, IMG_HEIGHT, image::FMT_BGR888);
        err::Err err;

        while (!app::need_exit())
        {
            image::Image *img = cam.read();
            if (!img)
            {
                log::warn("Camera read image failed!");
                delete img;
                continue;
            }
            
            disp.show(*img);
            delete img;
            thread::sleep_ms(10);
        }

        log::info("vision thread exit");
    }

} // namespace mVision