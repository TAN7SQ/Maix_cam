
#include "maix_basic.hpp"
#include "main.h"

#include "maix_display.hpp"

using namespace maix;
using namespace maix::display;

int _main(int argc, char *argv[])
{
    uint64_t t = time::time_s();
    log::info("Program start");

    // Run until app want to exit, for example app::switch_app API will set exit flag.
    // And you can also call app::set_exit_flag(true) to mark exit.

    Display disp;

    if (!disp.is_opened())
    {
        log::error("Failed to open display!");
        return -1;
    }

    image::Image img(disp.width(), disp.height(), disp.format());

    img.draw_string(10, 10, "hello world!", image::COLOR_BLUE, 1.0);
    img.draw_rect(50, 50, 200, 150, image::COLOR_GREEN, 2);
    disp.show(img);
    time::sleep_ms(5000);

    while (!app::need_exit())
    {
        log::info("%d", time::time_s());
        time::sleep(1);

        if (time::time_s() - t > 20)
            app::set_exit_flag(true);
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
