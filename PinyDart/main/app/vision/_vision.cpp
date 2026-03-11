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
                             true); // 实测只有单摄像头模式才能不裁切

    cam->exp_mode(maix::camera::AeMode::Manual);  // 设置为手动曝光模式
    cam->awb_mode(maix::camera::AwbMode::Manual); // 设置为手动白平衡模式

    cam->exposure(20);                              // 设置固定的曝光时间，单位us
    cam->set_wb_gain({0.12f, 0.07f, 0.07f, 0.11f}); // 设置固定的白平衡增益，顺序是r, gr, gb, b
    cam->gain(4096);                                // 设置固定的增益数值

    cam->vflip(1);   // 垂直翻转
    cam->hmirror(1); // 水平镜像

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

            if (!raw) {
                continue;
            }
            std::shared_ptr<image::Image> img(raw);
            frameQueue.push(img);

            cameraFps.tick();
            // Log::info(TAG, "%s", cameraFps.str());
        } catch (...) {
            Log::error(TAG, "camera read error");
        }
    }
}
void Vision::visionThread()
{
    Log::info(TAG, "vision thread start");
    std::vector<std::vector<int>> greenThreadshold = {{88, 92, -8, 20, -17, 0}}; // HSV绿色阈值范围

    while (!app::need_exit()) {

        auto img = frameQueue.pop();

        if (!img) {
            continue;
            maix::thread::sleep_ms(1);
        }
        try {
            auto out_img = img->copy();
            // out_img->binary(greenThreadshold);

            // ===== 在这里写视觉算法 =====
            // detect target / blob / tracking

            // Log::info(TAG, "%s", visonFps.str());

            std::shared_ptr<image::Image> out(out_img);
            recordQueue.push(out);
            visonFps.tick();
            // delete gray;

        } catch (...) {
            Log::error(TAG, "vision process error");
        }
    }
}

/**
 *
 */
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <string>

float get_cpu_temp()
{
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open())
        return -1;

    int temp;
    file >> temp;

    return temp / 1000.0f;
}

void Vision::recoderThread()
{
    maix::thread::sleep_ms(100);
    Log::info(TAG, "recoder thread start");

    // FIXME:实测如果不连接电脑热点的话会卡死
    // FIXME:连了也会卡死，但时间会久一点
    const char *PC_IP = "10.104.30.100";
    const int PORT = 5000;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK); // 设置套接字为非阻塞模式

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(PC_IP);

    uint64_t last_send = 0;

    const int TARGET_FPS = 20;
    const int FRAME_INTERVAL = 1000 / TARGET_FPS;

    while (!app::need_exit()) {

        auto img = recordQueue.pop_latest();

        if (!img) {
            maix::thread::sleep_ms(1);
            continue;
        }

        uint64_t now = time::ticks_ms();

        if (now - last_send < FRAME_INTERVAL) {
            continue;
        }

        last_send = now;

        image::Image *jpeg = nullptr;

        try {
            img->draw_string(10, 10, (std::string("C:") + cameraFps.str()).c_str(), image::Color(255, 0, 0));
            img->draw_string(10, 20, (std::string("V:") + visonFps.str()).c_str(), image::Color(255, 0, 0));
            float t = get_cpu_temp();
            char buf[64];
            sprintf(buf, "T: %.1fC", t);
            img->draw_string(10, 30, buf, image::Color(255, 0, 0));

            jpeg = img->to_format(image::Format::FMT_JPEG);
            if (!jpeg) {
                delete jpeg;
                continue;
            }

            uint8_t *data = (uint8_t *)jpeg->data();
            int size = jpeg->data_size();
            sendto(sock, data, size, 0, (struct sockaddr *)&addr, sizeof(addr));

            delete jpeg;
        } catch (...) {
            Log::error(TAG, "Recorder error");
        }
    }
}

// void Vision::recoderThread_just_record_mp4()
// {

//     maix::thread::sleep_ms(100);
//     Log::info(TAG, "recoder thread start");

//     static int skip = 0;

//     video::Encoder enc("/root/test.mp4",
//                        IMG_WIDTH, // 这个是编码格式，调用下方的resize函数调整到这个尺寸，编码效率更高
//                        IMG_HEIGHT,
//                        image::Format::FMT_YVU420SP,
//                        video::VideoType::VIDEO_H264,
//                        20, // fps
//                        50,
//                        1200 * 1000, // bitrate
//                        1000,
//                        false,
//                        true);

//     while (!app::need_exit()) {

//         auto img = recordQueue.pop();

//         if (!img)
//             continue;

//         skip++;

//         if (skip % 4 != 0)
//             continue;

//         try {
//             img->draw_string(10, 10, fps.str(), image::Color(255, 0, 0));
//             img->draw_rect(0, 0, img->width(), img->height(), image::Color(255, 0, 0), 5);

//             auto yuv = img->to_format(image::Format::FMT_YVU420SP);

//             video::Frame *frame = enc.encode(yuv);

//             delete yuv;
//             delete frame;
//         } catch (...) {
//             Log::error(TAG, "encode error");
//         }
//     }
// }

Vision::~Vision()
{
    delete cam;

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
