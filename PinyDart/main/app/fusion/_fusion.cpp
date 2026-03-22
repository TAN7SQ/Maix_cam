#include "main.h"
#include "maix_basic.hpp"

#include "_fusion.hpp"
#include "_vision.hpp"

using namespace maix;

void Fusion::fusionSchedule(const FusionConfig &config)
{
    this->_config = config;

    if (pFusionThread == nullptr) {
        pFusionThread = new std::thread(&Fusion::fusionThread, this);
        pthread_setname_np(pFusionThread->native_handle(), "FusionThread");
    }
}

void Fusion::fusionThread(void)
{
    log::info("fusion thread start");

    while (!app::need_exit()) {
        // TODO：融合逻辑
        // TODO: 带姿态补偿的LOS

        maix::thread::sleep_ms(1);
        camAim = targetQueue.pop();
    }

    log::info("fusion thread exit");
}
