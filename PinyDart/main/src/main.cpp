
#include "main.h"

#include "application.hpp"
#include "_basic.hpp"

using namespace maix;

int _main(int argc, char *argv[])
{

    log::info("Program start");

    App app = App();
    app.appSchedule();

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
