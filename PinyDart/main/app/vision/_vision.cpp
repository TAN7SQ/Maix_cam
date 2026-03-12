#include "_vision.hpp"

#include "_easyLog.hpp"

using namespace maix;

void Vision::visionSchedule(void)
{

    static camera::Camera pCam(IMG_WIDTH,
                               IMG_HEIGHT, //
                               image::Format::FMT_RGB888,
                               nullptr,
                               CAM_FPS,
                               3,
                               true,
                               true); // 实测只有单摄像头模式才能不裁切

    if (!pCam.is_opened()) {
        printf("Camera open failed!\n");
    }

    pCam.exp_mode(maix::camera::AeMode::Manual);
    // // pCam.awb_mode(maix::camera::AwbMode::Manual);
    pCam.exposure(200);
    pCam.iso(20);

    pCam.vflip(1);   // 垂直翻转
    pCam.hmirror(1); // 水平镜像

    // Log::warn(TAG, "exp:%d,gain:%d", pCam.exposure(-1), pCam.gain(-1));

    if (pCameraThread == nullptr) {
        pCameraThread = new std::thread(&Vision::cameraThread, this, &pCam);
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

/**
 *
 */
void Vision::cameraThread(camera::Camera *pcam)
{
    Log::info(TAG, "camera thread start");
    pcam->skip_frames(10);

    while (!app::need_exit()) {
        try {
            maix::image::Image *raw = pcam->read();

            if (!raw) {
                maix::thread::sleep_ms(1);
                continue;
            }

            std::shared_ptr<image::Image> img(raw);

            frameQueue.push(img);
            // delete raw;

            cameraFps.tick();
            maix::thread::sleep_ms(2);

        } catch (...) {
            Log::error(TAG, "camera read error");
        }
    }
}

/**
 *
 */
void Vision::visionThread()
{
    Log::info(TAG, "vision thread start");

    std::vector<std::vector<int>> greenThresholds = {{0, 80, -120, -10, 0, 30}};

    while (!app::need_exit()) {
        auto img = frameQueue.pop();

        if (!img) {
            maix::thread::sleep_ms(1);
            continue;
        }
        auto new_img = std::shared_ptr<image::Image>(img->copy());

        try {
            // auto blobs = img->find_blobs(
            //     greenThresholds, false, {0, 0, img->width(), img->height()}, 2, 2, 50, 50, true, 10, 16, 16);

            // for (auto &blob : blobs) {
            //     img->draw_rect(blob.x(), blob.y(), blob.w(), blob.h(), maix::image::COLOR_RED, 2);
            // }

            new_img->draw_string(10, 10, (std::string("C:") + cameraFps.str()).c_str(), image::Color(255, 0, 0));
            new_img->draw_string(10, 22, (std::string("V:") + visonFps.str()).c_str(), image::Color(255, 0, 0));
            new_img->draw_string(10, 34, Temp::get_cpu_temp().c_str(), image::Color(255, 0, 0));

            recordQueue.push(new_img);
            visonFps.tick();
            maix::thread::sleep_ms(2);

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

void Vision::recoderThread()
{
    maix::thread::sleep_ms(100);

    Log::info(TAG, "recoder thread start");

    const char *PC_IP = "10.104.30.100";
    const int PORT = 5000;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    fcntl(sock, F_SETFL, O_NONBLOCK);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(PC_IP);

    uint64_t last_send = 0;

    const int TARGET_FPS = 15;
    const int FRAME_INTERVAL = 1000 / TARGET_FPS;

    while (!app::need_exit()) {
        auto img = recordQueue.pop();

        if (!img) {
            maix::thread::sleep_ms(1);
            continue;
        }

        uint64_t now = time::ticks_ms();

        if (now - last_send < FRAME_INTERVAL)
            continue;

        last_send = now;

        try {
            // auto rgb = img->to_format(image::Format::FMT_RGB888);

            image::Image *jpeg = img->to_jpeg(60);
            if (!jpeg)
                continue;
            uint8_t *data = (uint8_t *)jpeg->data();
            int size = jpeg->data_size();
            sendto(sock, data, size, 0, (struct sockaddr *)&addr, sizeof(addr));

            delete jpeg;
            // delete rgb;
        } catch (...) {
            Log::error(TAG, "recorder error");
        }
        maix::thread::sleep_ms(1);
    }
}

// void Vision::recoderThread()
// {
//     maix::thread::sleep_ms(100);
//     Log::info(TAG, "recoder thread start");

//     // FIXME:实测如果不连接电脑热点的话会卡死
//     // FIXME:连了也会卡死，但时间会久一点
//     const char *PC_IP = "10.104.30.100";
//     const int PORT = 5000;

//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     fcntl(sock, F_SETFL, O_NONBLOCK); // 设置套接字为非阻塞模式

//     struct sockaddr_in addr;
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(PORT);
//     addr.sin_addr.s_addr = inet_addr(PC_IP);

//     uint64_t last_send = 0;

//     const int TARGET_FPS = 20;
//     const int FRAME_INTERVAL = 1000 / TARGET_FPS;

//     while (!app::need_exit()) {

//         auto img = recordQueue.pop_latest();

//         if (!img) {
//             maix::thread::sleep_ms(1);
//             continue;
//         }

//         uint64_t now = time::ticks_ms();

//         if (now - last_send < FRAME_INTERVAL) {
//             continue;
//         }

//         last_send = now;

//         image::Image *jpeg = nullptr;

//         try {
//             img->draw_string(10, 10, (std::string("C:") + cameraFps.str()).c_str(), image::Color(255, 0, 0));
//             img->draw_string(10, 22, (std::string("V:") + visonFps.str()).c_str(), image::Color(255, 0, 0));
//             float t = get_cpu_temp();
//             char buf[64];
//             sprintf(buf, "T: %.1f", t);
//             img->draw_string(10, 34, buf, image::Color(255, 0, 0));

//             jpeg = img->to_jpeg(50);
//             if (!jpeg) {
//                 delete jpeg;
//                 continue;
//             }

//             uint8_t *data = (uint8_t *)jpeg->data();
//             int size = jpeg->data_size();
//             sendto(sock, data, size, 0, (struct sockaddr *)&addr, sizeof(addr));

//             delete jpeg;
//         } catch (...) {
//             Log::error(TAG, "Recorder error");
//         }
//     }
// }

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
