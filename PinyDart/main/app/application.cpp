#include "application.hpp"

using namespace maix;

#include "_easyLog.hpp"
#include "vision/_vision.hpp"
void App::appInit()
{

    Log::init();
    Log::info("App", "App init");
}

void App::appSchedule()
{
    App::appInit();

    Vision vision = Vision();
    vision.visionSchedule();

    while (!app::need_exit()) {
        Log::info("App", "App running...%d", 0);
        Log::error("App", "App running...%d", 0);
        Log::debug("App", "App running...%d", 0);
        Log::trace("App", "App running...%d", 0);
        Log::warn("App", "App running...%d", 0);

        maix::thread::sleep_ms(1000);
    }
}
