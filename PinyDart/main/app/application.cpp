#include "application.hpp"
#include "uart.hpp"

using namespace maix;

#include "_easyLog.hpp"
#include "vision/_vision.hpp"

#include <signal.h>

#include "_configJson.hpp"

#include "_shared.hpp"

SharedQueue<Target> globalTargetQueue;

void App::appInit(int argc, char *argv[])
{

    Log::init();
    Log::info("App", "App init");
    /********************************************************* */

    std::string config_path = "/root/config/config.json";
    if (argc >= 2) {
        config_path = argv[1];
        Log::info("App", "use custom config: %s", config_path.c_str());
    }
    else
        Log::info("App", "use default config");

    if (!ConfigJson::load(config_path, config)) {
        Log::error("App", "load config file failed: %s", config_path.c_str());
        return;
    }
    /********************************************************* */
}

void App::appSchedule(int argc, char *argv[])
{
    App::appInit(argc, argv);

    /********************************************************* */
    Uart uart1(Uart::UART1, 1500000);
    uart1.uartSchedule();

    Vision vision = Vision(globalTargetQueue);
    vision.visionSchedule(config.vision);

    while (!app::need_exit()) {
        maix::thread::sleep_ms(100);
    }
}
