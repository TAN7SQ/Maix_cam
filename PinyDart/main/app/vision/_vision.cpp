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
    pCam.constrast(100);
    pCam.iso(15);

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

    while (!app::need_exit()) {
        auto img = frameQueue.pop();

        if (!img) {
            maix::thread::sleep_ms(1);
            continue;
        }
        auto new_img = std::shared_ptr<image::Image>(img->copy());

        try {
            Vision::targetDetect(new_img);
            Vision::debugInfo(new_img);

            recordQueue.push(new_img);
            visonFps.tick();
            maix::thread::sleep_ms(2);

        } catch (...) {
            Log::error(TAG, "vision process error");
        }
    }
}

/**
 * 检测所有像素
 */
float Vision::calcBlobBrightness(maix::image::Image *img, maix::image::Blob &blob)
{
    int sum = 0;
    int count = 0;
    const int step = 2;
    for (int y = blob.y(); y < blob.y() + blob.h(); y += step) {
        for (int x = blob.x(); x < blob.x() + blob.w(); x += step) {
            auto c = img->get_pixel(x, y);

            int r = c[0];
            int g = c[1];
            int b = c[2];

            int yv = (299 * r + 587 * g + 114 * b) / 1000;

            sum += yv;
            count++;
        }
    }

    return (float)sum / count;
}

float Vision::calcBlobCenterBrightness(maix::image::Image *img, maix::image::Blob &blob)
{
    int cx = blob.x() + blob.w() / 2;
    int cy = blob.y() + blob.h() / 2;

    const int radius = 2; // 5x5
    int sum = 0;
    int count = 0;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int x = cx + dx;
            int y = cy + dy;

            if (x < 0 || y < 0 || x >= img->width() || y >= img->height())
                continue;

            auto c = img->get_pixel(x, y);

            int r = c[0];
            int g = c[1];
            int b = c[2];

            // 过滤白色反光、黄色灯、噪声
            // if (g < r * 1.2 || g < b * 1.2)
            //     continue;
            int yv = (299 * r + 587 * g + 114 * b) / 1000;

            sum += yv;
            count++;
        }
    }

    if (count == 0)
        return 0;

    return (float)sum / count;
}

void Vision::targetDetect(std::shared_ptr<maix::image::Image> img)
{
    static std::vector<std::vector<int>> greenThresholds = {{40, 100, -120, -10, 0, 30}};
    // static std::vector<std::vector<int>> greenThresholds = {{0, 80, -120, -10, 0, 30}};

    // 下一步可以加上动态ROI
    const int layROI = 30;

    auto blobs = img->find_blobs(greenThresholds,
                                 false,
                                 {layROI, layROI, img->width() - layROI, img->height() - layROI},
                                 2,
                                 2,
                                 10,
                                 5,
                                 false,
                                 0,
                                 0,
                                 0);

    maxblob.pixels = 0;
    maxblob.brightness = 0;

    float bestScore = 0;

    for (auto &blob : blobs) {

        float density = blob.pixels() / float(blob.w() * blob.h());

        if (density > 0.9 && density < 0.25)
            continue;
        float brightness = calcBlobCenterBrightness(img.get(), blob);

        // 评价函数：亮度 * 像素数
        float score = brightness * brightness * blob.pixels();

        if (score > bestScore) {
            bestScore = score;

            maxblob.x = blob.x();
            maxblob.y = blob.y();
            maxblob.w = blob.w();
            maxblob.h = blob.h();
            maxblob.pixels = blob.pixels();
            maxblob.brightness = brightness;
            // printf("blob pixels=%d density=%.2f bright=%.1f\n", blob.pixels(), density, brightness);
        }
    }

    if (maxblob.pixels > 0) {
        int cx = maxblob.x + maxblob.w / 2;
        int cy = maxblob.y + maxblob.h / 2;
        img->draw_circle(cx, cy, maxblob.w / 2, maix::image::COLOR_RED);
        img->draw_cross(cx, cy, maix::image::COLOR_RED, 2);
    }
}

void Vision::debugInfo(std::shared_ptr<maix::image::Image> img)
{
    img->draw_string(10, 10, (std::string("C:") + cameraFps.str()).c_str(), image::COLOR_WHITE);
    img->draw_string(10, 22, (std::string("V:") + visonFps.str()).c_str(), image::COLOR_WHITE);
    img->draw_string(10, 34, Temp::get_cpu_temp().c_str(), image::COLOR_WHITE);
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

//----------------------------------------------------
// struct LedTarget
// {
//     int x = -1;
//     int y = -1;
//     int brightness = 0;
// };

// LedTarget Vision::detectGreenLED(maix::image::Image *img)
// {
//     LedTarget best;

//     const int step = 2; // 每2像素扫描一次，速度×4
//     const int w = img->width();
//     const int h = img->height();

//     for (int y = 10; y < h - 10; y += step) {
//         for (int x = 10; x < w - 10; x += step) {
//             auto c = img->get_pixel(x, y);

//             int r = c[0];
//             int g = c[1];
//             int b = c[2];

//             // 绿色过滤
//             if (g < r + 30)
//                 continue;
//             if (g < b + 30)
//                 continue;

//             // 亮度
//             int bright = (299 * r + 587 * g + 114 * b) / 1000;

//             if (bright < 120)
//                 continue;

//             // 找最亮
//             if (bright > best.brightness) {
//                 best.x = x;
//                 best.y = y;
//                 best.brightness = bright;
//             }
//         }
//     }

//     return best;
// }
// bool Vision::confirmLED(maix::image::Image *img, int cx, int cy)
// {
//     int count = 0;

//     for (int dy = -2; dy <= 2; dy++) {
//         for (int dx = -2; dx <= 2; dx++) {
//             auto c = img->get_pixel(cx + dx, cy + dy);

//             int r = c[0], g = c[1], b = c[2];

//             if (g > r + 20 && g > b + 20)
//                 count++;
//         }
//     }

//     return count > 8;
// }
// void Vision::targetDetect(std::shared_ptr<maix::image::Image> img)
// {
//     auto led = detectGreenLED(img.get());

//     if (led.brightness == 0)
//         return;

//     if (!confirmLED(img.get(), led.x, led.y))
//         return;

//     img->draw_cross(led.x, led.y, maix::image::COLOR_RED, 3);
//     img->draw_circle(led.x, led.y, 8, maix::image::COLOR_RED);
// }