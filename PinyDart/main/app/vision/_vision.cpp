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

        } catch (const std::exception &e) {
            Log::error(TAG, "cam exception: %s", e.what());
        } catch (...) {
            Log::error(TAG, "cam unknown error");
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

        } catch (const std::exception &e) {
            Log::error(TAG, "vision exception: %s", e.what());
        } catch (...) {
            Log::error(TAG, "vision unknown error");
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
//    uint64_t last_send = 0;
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
    int stride;
    if (maxblob.w > 20)
        stride = 3;
    else if (maxblob.w > 10)
        stride = 2;
    else
        stride = 1;

    int x, y, w, h;

    if (maxblob.pixels > 0) {

        int roi_size = std::clamp(maxblob.w * 4, 20, 100);

        x = std::clamp((int)maxblob.cx - roi_size / 2, 0, img->width() - 1);
        y = std::clamp((int)maxblob.cy - roi_size / 2, 0, img->height() - 1);

        w = std::min(roi_size, img->width() - x);
        h = std::min(roi_size, img->height() - y);

        if (w <= 0 || h <= 0) {
            x = 0;
            y = 0;
            w = img->width();
            h = img->height();
        }
    }
    else {
        x = 0;
        y = 0;
        w = img->width();
        h = img->height();
    }

    auto blobs = img->find_blobs(_config.find_blobs.green_thresholds,
                                 false,
                                 {x, y, w, h},
                                 stride,
                                 stride,
                                 _config.find_blobs.area_threshold,
                                 _config.find_blobs.pixels_threshold,
                                 _config.find_blobs.merge,
                                 _config.find_blobs.margin,
                                 _config.find_blobs.x_hist_bins_max,
                                 _config.find_blobs.y_hist_bins_max);

    maxblob.pixels = 0;
    maxblob.brightness = 0;
    maxblob.w = 0;

    float bestScore = 0;

    // *******************************************************

    for (auto &blob : blobs) {

        if (blob.pixels() < 5)
            continue;

        if (blob.w() > 60 || blob.h() > 60)
            continue;

        float brightness = calcBlobCenterBrightness(img.get(), blob);

        float score = brightness * brightness * sqrt(blob.pixels());

        if (score > bestScore) {
            auto sub = calcBlobSubpixelCenter(img.get(), blob);
            bestScore = score;

            maxblob.cx = cxLpf.update(sub.cx);
            maxblob.cy = cyLpf.update(sub.cy);
            // maxblob.cx = sub.cx;
            // maxblob.cy = sub.cy;
            maxblob.x = blob.x();
            maxblob.y = blob.y();
            maxblob.w = blob.w();
            maxblob.h = blob.h();
            maxblob.pixels = blob.pixels();
            maxblob.brightness = brightness;
        }
    }

    if (maxblob.pixels > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "C:%.1f, %.1f", maxblob.cx, maxblob.cy);
        img->draw_string(maxblob.cx, maxblob.cy, buf, maix::image::COLOR_RED);

        img->draw_circle(maxblob.cx, maxblob.cy, maxblob.w / 2, maix::image::COLOR_RED, 3);
        img->draw_cross(maxblob.cx, maxblob.cy, maix::image::COLOR_RED, 3);
    }
}

void Vision::debugInfo(std::shared_ptr<maix::image::Image> img)
{
    img->draw_string(10, 10, (std::string("C:") + cameraFps.str()).c_str(), image::COLOR_WHITE);
    img->draw_string(10, 22, (std::string("V:") + visonFps.str()).c_str(), image::COLOR_WHITE);
    img->draw_string(10, 34, Temp::get_cpu_temp().c_str(), image::COLOR_WHITE);
}

/**
 * @brief just for rgb888 image
 */
Vision::SubpixelResult Vision::calcBlobSubpixelCenter(maix::image::Image *img, maix::image::Blob &blob)
{
    int cx0 = blob.x() + blob.w() / 2;
    int cy0 = blob.y() + blob.h() / 2;
    const int radius = 3;
    float sumI = 0.0f;
    float sumX = 0.0f;
    float sumY = 0.0f;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {

            int x = cx0 + dx;
            int y = cy0 + dy;

            if (x < 0 || y < 0 || x >= img->width() || y >= img->height())
                continue;

            auto c = img->get_pixel(x, y, true);

            int r = c[0];
            int g = c[1];
            int b = c[2];

            float I = (299 * r + 587 * g + 114 * b) / 1000.0f;

            float green_weight = g - std::max(r, b);

            // if (green_weight < 10) {
            //     continue;
            // }

            // 加入权重（越绿权重越高）
            float weight = I * (1.0f + green_weight / 50.0f);

            sumI += weight;
            sumX += weight * x;
            sumY += weight * y;
        }
    }

    SubpixelResult res;

    // 避免“完全丢失”
    if (sumI < 1e-3f) {
        res.cx = cx0;
        res.cy = cy0;
        res.brightness = 0;
        return res;
    }

    res.cx = sumX / sumI;
    res.cy = sumY / sumI;

    // 用平均亮度
    res.brightness = sumI / ((2 * radius + 1) * (2 * radius + 1));

    return res;
}