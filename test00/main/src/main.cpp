
#include "maix_basic.hpp"
#include "main.h"

#include "maix_display.hpp"
#include "maix_camera.hpp"

using namespace maix;

int _main(int argc, char *argv[])
{
    display::Display disp = display::Display();
    disp.set_backlight(50);

    camera::Camera cam = camera::Camera(320, 240, image::Format::FMT_GRAYSCALE);
    cam.skip_frames(30); // 跳过开头的30帧

    // Run until app want to exit, for example app::switch_app API will set exit flag.
    // And you can also call app::set_exit_flag(true) to mark exit.
    while (!app::need_exit())
    {
        image::Image *img = cam.read();

        img->draw_rect(10, 10, 10, 10, image::COLOR_BLUE);
        img->draw_string(20, 20, "hello world tan", image::COLOR_GREEN, 1);

        disp.show(*img);
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
