
#include "maix_basic.hpp"
#include "main.h"

#include "opencv2/opencv.hpp"
#include "opencv2/freetype.hpp"

#include "maix_display.hpp"
#include "maix_camera.hpp"
#include "maix_image_cv.hpp"

using namespace maix;

int _main(int argc, char *argv[])
{
    display::Display disp = display::Display();
    disp.set_backlight(50);

    camera::Camera cam = camera::Camera(320, 240);
    cam.skip_frames(30); // 跳过开头的30帧

    err::Err err;

    //---------------------------------------------------
    // 3. 定义红色在 HSV 中的范围（分两段，因为红色跨越 0°）
    cv::Scalar lower_read1(0, 40, 40);
    cv::Scalar upper_read1(10, 255, 255);
    cv::Scalar lower_read2(170, 40, 40);
    cv::Scalar upper_read2(180, 255, 2555);

    cv::Mat mask1, mask2, mask3;
    //---------------------------------------------------

    // Run until app want to exit, for example app::switch_app API will set exit flag.
    // And you can also call app::set_exit_flag(true) to mark exit.
    while (!app::need_exit())
    {
        image::Image *img = cam.read();

        cv::Mat cv_mat;

        //---------------------------------------------------------
        err = image::image2cv(*img, cv_mat, false, false);
        cv::inRange(cv_mat, lower_read1, upper_read1, mask1);
        cv::inRange(cv_mat, lower_read2, upper_read2, mask2);
        cv::bitwise_or(mask1, mask2, mask3);

        // 形态学处理，去除噪声，填充小孔
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
        cv::morphologyEx(mask3, mask3, cv::MORPH_CLOSE, kernel); // 闭运算：填充内部空间
        cv::morphologyEx(mask3, mask3, cv::MORPH_OPEN, kernel);  // 开运算：去除小噪声

        // 查找轮廓
        std::vector<vector<cv::Point>> contours;
        cv::findContours(mask3, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 遍历轮廓并框选红色区域
        for (size_t i = 0; i < contours.size(); i++)
        {
            double area = cv::contourArea(contours[i]);
            if (area > 50) // 面积大于500的轮廓
            {
                cv::Rect bound_rect = cv::boundingRect(contours[i]);
                cv::rectangle(cv_mat, bound_rect.tl(), bound_rect.br(), cv::Scalar(255, 0, 0), 2);
            }
        }
        //---------------------------------------------------------
        image::Image *cvimg = 0;
        cvimg = image::cv2image(cv_mat, false, false);

        disp.show(*cvimg);
        delete img;
        delete cvimg;
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
