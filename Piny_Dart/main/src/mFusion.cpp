#include "mVision.hpp"
#include "maix_basic.hpp"
#include "main.h"

#include "mFusion.hpp"
#include "kalmanFilter.hpp"

using namespace maix;

namespace mFusion
{
    void fusion_thread(void)
    {
        log::info("fusion thread start");

        KalmanFilter kf;

        while (!app::need_exit())
        {
            // TODO：融合逻辑

            test_kalman();

            maix::thread::sleep_ms(10);
        }

        log::info("fusion thread exit");
    }

} // namespace mFusion
