#include "_vision.hpp"

#include "_easyLog.hpp"

using namespace maix;

void Vision::visionSchedule(void)
{
    cam = new camera::Camera(IMG_WIDTH,
                             IMG_HEIGHT, //
                             image::Format::FMT_RGB888,
                             nullptr,
                             CAM_FPS,
                             3,
                             true,
                             true);

    // camRecord = cam->add_channel(IMG_WIDTH, IMG_HEIGHT, image::Format::FMT_RGB888, CAM_FPS, 3, true);

    if (pCameraThread == nullptr) {
        pCameraThread = new std::thread(&Vision::cameraThread, this, cam);
        pthread_setname_np(pCameraThread->native_handle(), "cameraThread");
    }
    if (pVisionThread == nullptr) {
        pVisionThread = new std::thread(&Vision::visionThread, this);
        pthread_setname_np(pVisionThread->native_handle(), "visionThread");
    }
    if (pRecoderThread == nullptr) {
        pRecoderThread = new std::thread(&Vision::recoderThread, this);
        pthread_setname_np(pRecoderThread->native_handle(), "recoderThread");
    }
}
void Vision::cameraThread(camera::Camera *cam)
{

    Log::info(TAG, "camera thread start");

    while (!app::need_exit()) {
        try {
            maix::image::Image *raw = cam->read();

            if (!raw)
                continue;
            std::shared_ptr<image::Image> img(raw);
            frameQueue.push(img);

            fps.tick();
            Log::info(TAG, "%s", fps.str());
        } catch (...) {
            Log::error(TAG, "camera read error");
        }
    }
}
void Vision::visionThread()
{
    Log::info(TAG, "vision thread start");

    while (!app::need_exit()) {

        auto img = frameQueue.pop();

        if (!img)
            continue;

        try {
            auto gray = img->to_format(image::Format::FMT_GRAYSCALE);

            // ===== 在这里写视觉算法 =====
            // detect target / blob / tracking

            std::shared_ptr<image::Image> out(gray);
            recordQueue.push(out);

        } catch (...) {
            Log::error(TAG, "vision process error");
        }
    }
}

void Vision::recoderThread()
{

    maix::thread::sleep_ms(100);
    Log::info(TAG, "recoder thread start");

    static int skip = 0;

    video::Encoder enc("/root/test.mp4",
                       IMG_WIDTH, // 这个是编码格式，调用下方的resize函数调整到这个尺寸，编码效率更高
                       IMG_HEIGHT,
                       image::Format::FMT_YVU420SP,
                       video::VideoType::VIDEO_H264,
                       20, // fps
                       50,
                       1200 * 1000, // bitrate
                       1000,
                       false,
                       true);

    while (!app::need_exit()) {

        auto img = recordQueue.pop();

        if (!img)
            continue;

        skip++;

        if (skip % 4 != 0)
            continue;

        try {
            img->draw_string(10, 10, fps.str(), image::Color(255, 0, 0));
            img->draw_rect(0, 0, img->width(), img->height(), image::Color(255, 0, 0), 5);

            auto yuv = img->to_format(image::Format::FMT_YVU420SP);

            video::Frame *frame = enc.encode(yuv);

            delete yuv;
            delete frame;
        } catch (...) {
            Log::error(TAG, "encode error");
        }
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
    if (pRecoderThread) {
        if (pRecoderThread->joinable()) {
            pRecoderThread->join();
        }
        delete pRecoderThread;
        pRecoderThread = nullptr;
    }
    if (pCameraThread) {
        if (pCameraThread->joinable())
            pCameraThread->join();
        delete pCameraThread;
    }

    Log::warn(TAG, "vision thread destroy");
}
