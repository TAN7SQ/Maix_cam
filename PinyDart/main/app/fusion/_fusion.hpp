#ifndef __MFUSION_HPP__
#define __MFUSION_HPP__

#pragma once
#include "_configJson.hpp"

#include "_basic.hpp"

#include "_shared.hpp"

class Fusion
{
public:
    static constexpr const char *TAG = "fusion";

    void fusionSchedule(const FusionConfig &config);

    Fusion(SharedQueue<CamTargetData> &_camtargetQueue) //
        : targetQueue(_camtargetQueue) {};
    ~Fusion();

    void deThread();

private:
    SharedQueue<CamTargetData> &targetQueue;
    // camAim有目标经过归一化后的输出值，以及对应的yaw和pitch值
    CamTargetData camAim;
    FusionConfig _config;

    std::thread *pFusionThread = nullptr;

    /************************************* */
private:
    void fusionThread(void);
};
#endif // __MFUSION_HPP__