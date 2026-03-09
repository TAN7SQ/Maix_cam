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
    Log::info(TAG, "vision thread start");

    // display::Display disp = display::Display();
    camera::Camera cam(IMG_WIDTH, IMG_HEIGHT, image::Format::FMT_YVU420SP);
    video::Encoder enc("/root/test.mp4",
                       IMG_WIDTH,
                       IMG_HEIGHT,
                       image::Format::FMT_YVU420SP,
                       video::VideoType::VIDEO_H264,
                       30,
                       50,
                       3000 * 1000,
                       1000,
                       false,
                       true);

    err::Err err;

    while (!app::need_exit()) {
        image::Image *img = cam.read();

        if (!img) {
            delete img;
            continue;
        }
        Log::info(TAG, "%s", fps.str());

        // TODO：视觉识别逻辑

        // disp.show(*img);

        /*********************内录逻辑 **********************/

        video::Frame *frame = enc.encode(img);
        delete frame;
        // image::Image *enc_img = enc.capture();
        // Log::info(TAG,
        //           "capture image: %d x %d, format: %d, data size: %d",
        //           enc_img->width(),
        //           enc_img->height(),
        //           enc_img->format(),
        //           enc_img->data_size());
        delete img;
        // maix::thread::sleep_ms(1);
        fps.tick();
    }
}

Vision::~Vision()
{
    if (pVisionThread) {
        if (pVisionThread->joinable()) {
            pVisionThread->join();
        }
        delete pVisionThread;
        pVisionThread = nullptr;
    }
    Log::warn(TAG, "vision thread destroy");
}
