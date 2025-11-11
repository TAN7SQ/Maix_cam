
#include "maix_basic.hpp"
#include "main.h"

#include "opencv2/opencv.hpp"
#include "opencv2/freetype.hpp"

#include "maix_display.hpp"
#include "maix_camera.hpp"
#include "maix_image_cv.hpp"

using namespace maix;
using namespace cv;

#include "maix_basic.hpp"
#include "main.h"
#include "opencv2/opencv.hpp"
#include "maix_display.hpp"
#include "maix_camera.hpp"
#include "maix_image_cv.hpp"

using namespace maix;
using namespace cv;

int _main(int argc, char *argv[])
{
    display::Display disp = display::Display();
    // 相机初始化：指定BGR格式，避免通道转换错误
    camera::Camera cam = camera::Camera(320, 240, image::FMT_BGR888);
    err::Err err;

    cam.skip_frames(30); // 跳过开头的30帧

    while (!app::need_exit())
    {
        // 1. 读取图像，检查是否为空
        image::Image *img = cam.read();
        if (!img)
        {
            log::warn("相机读取图像失败！");
            continue; // 跳过本次循环，避免访问空指针
        }

        // 2. 转换为OpenCV Mat
        cv::Mat cv_mat;
        err = image::image2cv(*img, cv_mat);

        // 3. 颜色空间转换（BGR -> HSV）
        cv::Mat hsv_mat;
        cv::cvtColor(cv_mat, hsv_mat, cv::COLOR_BGR2HSV);

        // 4. 蓝色阈值分割（用单独的blue_mask存储掩码，避免覆盖hsv_mat）
        cv::Scalar lower_blue = cv::Scalar(100, 100, 0); // H S V
        cv::Scalar upper_blue = cv::Scalar(120, 255, 150);
        cv::Mat blue_mask;
        cv::inRange(hsv_mat, lower_blue, upper_blue, blue_mask);

        // 5. 形态学操作（优化掩码）
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(blue_mask, blue_mask, cv::MORPH_CLOSE, kernel); // 先膨胀后腐蚀，填充空洞

        // 6. 查找轮廓（关键：contours在每次循环中重新初始化，避免累积）
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(blue_mask, contours, hierarchy,
                         cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 7. 遍历轮廓，绘制旋转矩形（先检查contours是否为空）
        if (!contours.empty())
        {
            for (size_t i = 0; i < contours.size(); i++)
            {
                cv::Rect bound_rect = cv::boundingRect(contours[i]);
                // 过滤掉过小的轮廓
                if (bound_rect.area() > 600)
                {
                    cv::rectangle(cv_mat,
                                  bound_rect.tl(), bound_rect.br(),
                                  cv::Scalar(0, 255, 0), 2);
                }
            }
        }

        // 8. 显示结果，释放内存
        image::Image *out_img = image::cv2image(cv_mat);
        disp.show(*out_img);

        // 必须释放动态分配的图像指针，避免内存泄漏
        delete img;
        delete out_img;
    }

    log::info("Program exit");
    return 0;
}

int main(int argc, char *argv[])
{
    // Catch signal and process
    sys::register_default_signal_handle();

    // Use CATCH_EXCEPTION_RUN_RETURN to catch exception,
    // if we don't catch exception, when program throw exception, the objects will not be destructed.
    // So we catch exception here to let resources be released(call objects' destructor) before exit.
    CATCH_EXCEPTION_RUN_RETURN(_main, -1, argc, argv);
}
