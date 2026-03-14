#include "_vision.hpp"

#include "_easyLog.hpp"

using namespace maix;

void Vision::visionSchedule(const VisionConfig &config)
{
    // json data
    _config = config;

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
    pCam.exposure(200);
    pCam.constrast(100);
    pCam.iso(30);

    pCam.vflip(1);
    pCam.hmirror(1);

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
 ██████╗ █████╗ ███╗   ███╗███████╗██████╗  █████╗
██╔════╝██╔══██╗████╗ ████║██╔════╝██╔══██╗██╔══██╗
██║     ███████║██╔████╔██║█████╗  ██████╔╝███████║
██║     ██╔══██║██║╚██╔╝██║██╔══╝  ██╔══██╗██╔══██║
╚██████╗██║  ██║██║ ╚═╝ ██║███████╗██║  ██║██║  ██║
 ╚═════╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝
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
    delete pcam;
}

/**
██╗   ██╗██╗███████╗██╗ ██████╗ ███╗   ██╗
██║   ██║██║██╔════╝██║██╔═══██╗████╗  ██║
██║   ██║██║███████╗██║██║   ██║██╔██╗ ██║
╚██╗ ██╔╝██║╚════██║██║██║   ██║██║╚██╗██║
 ╚████╔╝ ██║███████║██║╚██████╔╝██║ ╚████║
  ╚═══╝  ╚═╝╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═══╝
 */
void Vision::visionThread()
{
    maix::thread::sleep_ms(100);

    Log::info(TAG, "vision thread start");
    ConfigJson::print_vision(_config);

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
██████╗ ███████╗ ██████╗ ██████╗ ██████╗ ██████╗ ███████╗██████╗
██╔══██╗██╔════╝██╔════╝██╔═══██╗██╔══██╗██╔══██╗██╔════╝██╔══██╗
██████╔╝█████╗  ██║     ██║   ██║██████╔╝██║  ██║█████╗  ██████╔╝
██╔══██╗██╔══╝  ██║     ██║   ██║██╔══██╗██║  ██║██╔══╝  ██╔══██╗
██║  ██║███████╗╚██████╗╚██████╔╝██║  ██║██████╔╝███████╗██║  ██║
╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝
 */

void Vision::recoderThread()
{
    maix::thread::sleep_ms(100);
    Log::info(TAG, "recoder thread start");

    uint64_t last_send = 0;

    const int TARGET_FPS = 15;
    const int FRAME_INTERVAL = 1000 / TARGET_FPS;
    /***************************************************** */
    if (_config.udp.is_enabled) {
        const char *PC_IP = _config.udp.udp_ip.c_str();
        const int PORT = _config.udp.udp_port;

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        fcntl(sock, F_SETFL, O_NONBLOCK);

        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = inet_addr(PC_IP);
        Log::info(TAG, "upd start");
    }
    /***************************************************** */
    // std::unique_ptr<video::Encoder> enc;

    // if (_config.mp4.is_enabled) {
    //     enc = std::make_unique<video::Encoder>(_config.mp4.mp4_path.c_str(),
    //                                            IMG_WIDTH,
    //                                            IMG_HEIGHT,
    //                                            image::Format::FMT_YVU420SP,
    //                                            video::VideoType::VIDEO_H264,
    //                                            _config.mp4.fps,
    //                                            50,
    //                                            _config.mp4.bitrate,
    //                                            1000,
    //                                            false,
    //                                            true);
    //     Log::info(TAG, "mp4 start");
    // }

    while (!app::need_exit()) {

        auto img = recordQueue.pop();
        if (!img) {
            maix::thread::sleep_ms(1);
            continue;
        }

        uint64_t now = time::ticks_ms();
        if (now - last_send < FRAME_INTERVAL) {
            maix::thread::sleep_ms(2);
            continue;
        }

        last_send = now;
        try {
            if (_config.udp.is_enabled) {
                std::unique_ptr<image::Image> jpeg(img->to_jpeg(30));
                if (jpeg) {
                    int ret = sendto(sock, jpeg->data(), jpeg->data_size(), 0, (struct sockaddr *)&addr, sizeof(addr));
                    if (ret < 0)
                        Log::warn(TAG, "sendto error %d", ret);
                }
            }

            // maix::thread::sleep_ms(1);
            // if (_config.mp4.is_enabled && enc) {
            //     auto yuv = img->to_format(image::Format::FMT_YVU420SP);
            //     video::Frame *frame = enc->encode(yuv); // 这里已经顺便内录了

            //     // /********************* */
            //     // if (frame && _config.udp.is_enabled) {
            //     //     static uint16_t frame_id = 0;
            //     //     const int PACKET = 1400;
            //     //     const int HEADER = 8;
            //     //     const int PAYLOAD = PACKET - HEADER;

            //     //     uint8_t *data = frame->data();
            //     //     int size = frame->size();
            //     //     int total = (size + PAYLOAD - 1) / PAYLOAD;

            //     //     for (int i = 0; i < total; i++) {
            //     //         uint8_t packet[PACKET];

            //     //         uint16_t *h = (uint16_t *)packet;
            //     //         h[0] = frame_id;
            //     //         h[1] = i;
            //     //         h[2] = total;
            //     //         h[3] = std::min(PAYLOAD, size - i * PAYLOAD);

            //     //         memcpy(packet + HEADER, data + i * PAYLOAD, h[3]);
            //     //         sendto(sock, packet, h[3] + HEADER, 0, (struct sockaddr *)&addr, sizeof(addr));
            //     //     }
            //     //     frame_id++;
            //     // }
            //     // /********************* */

            //     delete yuv;
            //     delete frame;
            // }
        } catch (...) {
            Log::error(TAG, "recoder thread error");
        }
        maix::thread::sleep_ms(1);
    }
}

// void Vision::recoderThread()
// {
//     maix::thread::sleep_ms(100);
//     Log::info(TAG, "recoder thread start");

//     uint64_t last_send = 0;

//     const int TARGET_FPS = 15;
//     const int FRAME_INTERVAL = 1000 / TARGET_FPS;
//     /***************************************************** */

//     const char *PC_IP = _config.udp.udp_ip.c_str();
//     const int PORT = _config.udp.udp_port;

//     sock = socket(AF_INET, SOCK_DGRAM, 0);

//     fcntl(sock, F_SETFL, O_NONBLOCK);

//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(PORT);
//     addr.sin_addr.s_addr = inet_addr(PC_IP);

//     /***************************************************** */

//     while (!app::need_exit()) {
//         auto img = recordQueue.pop();
//         if (!img) {
//             maix::thread::sleep_ms(1);
//             continue;
//         }
//         uint64_t now = time::ticks_ms();
//         if (now - last_send < FRAME_INTERVAL)
//             continue;
//         last_send = now;
//         try {
//             if (_config.udp.is_enabled) {
//                 image::Image *jpeg = img->to_jpeg(30);
//                 if (!jpeg)
//                     continue;
//                 uint8_t *data = (uint8_t *)jpeg->data();
//                 int size = jpeg->data_size();
//                 sendto(sock, data, size, 0, (struct sockaddr *)&addr, sizeof(addr));

//                 delete jpeg;
//             }

//         } catch (...) {
//             Log::error(TAG, "recorder error");
//         }
//         maix::thread::sleep_ms(1);
//     }
// }

void Vision::recoderThread_just_record_mp4(void)
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
            auto yuv = img->to_format(image::Format::FMT_YVU420SP);
            video::Frame *frame = enc.encode(yuv);
            delete yuv;
            delete frame;
        } catch (...) {
            Log::error(TAG, "encode error");
        }
    }
}

/********************************************************************* */

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

    const int radius = 3; // 5x5
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

    const int layROI = 10;
    // static std::vector<std::vector<int>> greenThresholds = {{0, 80, -120, -10, 0, 30}};

    auto blobs = img->find_blobs(_config.find_blobs.green_thresholds,
                                 false,
                                 {layROI, layROI, img->width() - layROI, img->height() - layROI},
                                 2,
                                 2,
                                 10,
                                 5,
                                 true,
                                 10,
                                 16,
                                 16);

    maxblob.pixels = 0;
    maxblob.brightness = 0;

    float bestScore = 0;

    static float last_best_score;

    for (auto &blob : blobs) {

        if (blob.pixels() < maxblob.pixels)
            continue;

        float brightness = Vision::calcBlobCenterBrightness(img.get(), blob);

        float score = brightness * brightness * blob.pixels();

        if (score > bestScore && score / last_best_score > 0.6) {
            bestScore = score;

            maxblob.x = blob.x();
            maxblob.y = blob.y();
            maxblob.w = blob.w();
            maxblob.h = blob.h();
            maxblob.pixels = blob.pixels();
            maxblob.brightness = brightness;
            // printf("blob pixels=%d brightness=%.2f\n", blob.pixels(), brightness);
        }
    }
    last_best_score = bestScore;

    if (maxblob.pixels > 0) {
        int cx = maxblob.x + maxblob.w / 2;
        int cy = maxblob.y + maxblob.h / 2;
        img->draw_circle(cx, cy, maxblob.w / 2, maix::image::COLOR_RED, 3);
        img->draw_cross(cx, cy, maix::image::COLOR_RED, 3);
    }
}

void Vision::debugInfo(std::shared_ptr<maix::image::Image> img)
{
    img->draw_string(10, 10, (std::string("C:") + cameraFps.str()).c_str(), image::COLOR_WHITE);
    img->draw_string(10, 22, (std::string("V:") + visonFps.str()).c_str(), image::COLOR_WHITE);
    img->draw_string(10, 34, Temp::get_cpu_temp().c_str(), image::COLOR_WHITE);
}
