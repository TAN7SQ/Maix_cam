
#include "maix_basic.hpp"
#include "main.h"

#include "maix_display.hpp"
#include "maix_camera.hpp"

using namespace maix;

// 帧率统计变量
static uint32_t cam_frame_cnt = 0;  // 摄像头采集帧数
static uint32_t disp_frame_cnt = 0; // 屏幕显示帧数
static uint64_t last_calc_time = 0; // 上次计算时间
static float cam_fps = 0.0f;        // 摄像头帧率
static float disp_fps = 0.0f;       // 屏幕帧率
static char fps_str[64] = {0};      // 帧率显示字符串

// 帧率计算核心函数（每1秒更新一次）
void update_fps()
{
    uint64_t curr_time = time::ticks_ms();
    uint64_t delta = curr_time - last_calc_time;

    if (delta >= 1000)
    {                                                       // 每1秒计算一次
        cam_fps = (float)cam_frame_cnt / (delta / 1000.0f); // 帧数/秒
        disp_fps = (float)disp_frame_cnt / (delta / 1000.0f);
        snprintf(fps_str, sizeof(fps_str), "Cam: %.1f | Disp: %.1f", cam_fps, disp_fps);

        // 输出到日志
        log::info("%s", fps_str);

        // 重置统计
        last_calc_time = curr_time;
        cam_frame_cnt = 0;
        disp_frame_cnt = 0;
    }
}

int _main(int argc, char *argv[])
{

    last_calc_time = time::ticks_ms(); // 初始化计时

    uint64_t t = time::time_s();
    log::info("Program start");

    err::Err error;

    camera::Camera cam = camera::Camera();
    display::Display disp = display::Display();

    error = cam.open();
    err::check_raise(error, "camera open failed");

    error = disp.open();
    err::check_raise(error, "display open failed");

    log::info("camera and display open success\n");
    log::info("screen size: %dx%d\n", disp.width(), disp.height());

    // Run until app want to exit, for example app::switch_app API will set exit flag.
    // And you can also call app::set_exit_flag(true) to mark exit.

    int angle = 0;    // 角度变量（用于动态图形）
    int x, y;         // 中心点坐标
    int r = 30;       // 圆形半径（占屏幕宽度的 30%）
    int block_h = 15; // 底部文字块高度（占屏幕高度的 15%）
    while (!app::need_exit())
    {
        image::Image *img = cam.read();
        err::check_null_raise(img, "camera read failed"); // 检查读取是否成功
        if (!img)
        {
            time::sleep_ms(5);
            continue;
        }

        cam_frame_cnt++;
        x = img->width() / 2;
        y = img->height() / 2;

        img->draw_circle(x, y, 2, image::Color::from_rgb(255, 255, 255), -1);

        // 计算旋转点坐标（基于角度的圆周运动）
        int x2 = x + r * img->width() / 100.0 * sin(angle * 3.14 / 180); // 正弦计算 x 偏移
        int y2 = y - r * img->width() / 100.0 * cos(angle * 3.14 / 180); // 余弦计算 y 偏移

        img->draw_circle(x2, y2, 10, image::Color::from_rgb(255, 255, 255), -1);

        // 角度递增（0-359 循环），实现动态旋转效果
        if (++angle == 360)
            angle = 0;

        // 绘制从中心点到旋转点的直线（白色，线宽 1）
        img->draw_line(x, y, x2, y2, image::Color::from_rgb(255, 255, 255), 1);

        // 计算底部文字块的 y 坐标（屏幕底部向上 15% 高度处）
        y = img->height() - block_h * img->height() / 100;

        // 绘制红色实心矩形作为文字背景（覆盖底部 15% 区域）
        img->draw_rect(0, y, img->width(), block_h * img->height() / 100,
                       image::Color::from_rgb(255, 0, 0), -1);

        // 计算文字尺寸（"Application"，字体大小 1.5）
        image::Size size = image::string_size("Application", 1.5);

        // 在红色背景中央绘制白色文字
        img->draw_string(
            (img->width() - size.width()) / 2,                       // 水平居中
            y + (block_h * img->height() / 100 - size.height()) / 2, // 垂直居中
            "Application",
            image::Color::from_rgb(255, 255, 255),
            1.5 // 字体大小
        );

        // 检查显示屏是否已关闭（主要用于 PC 端调试）
        if (!disp.is_opened())
        {
            log::info("disp closed\n");
            break;
        }

        // 将处理后的图像显示到屏幕上

        update_fps();
        img->draw_string(10, 10, fps_str, image::COLOR_WHITE, 1.2); // 显示到屏幕
        disp_frame_cnt++;                                           // 显示帧数+1
        disp.show(*img);

        // 释放图像内存（必须手动释放，避免内存泄露）
        delete img;
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
