#ifndef __MFUSION_HPP__
#define __MFUSION_HPP__

#pragma once
#include "_configJson.hpp"

#include "_basic.hpp"

#include "_shared.hpp"

struct BodyTarget
{
    bool valid = false;
    float x;           // 机体坐标系X（右）
    float y;           // 机体坐标系Y（上）
    float z;           // 机体坐标系Z（前）
    float yaw_error;   // 偏航误差（弧度）：正=目标在右侧，负=左侧
    float pitch_error; // 俯仰误差（弧度）：正=目标在上方，负=下方
    float roll_error;  // 横滚误差
};

struct ControlErr
{
    float yaw;   // 最终偏航控制误差（弧度）
    float pitch; // 最终俯仰控制误差（弧度）
    bool valid;  // 是否有效
};

class Fusion
{

public:
    static constexpr const char *TAG = "fusion";

    void fusionSchedule(const FusionConfig &config);

    Fusion() {};
    ~Fusion() {};

    void deThread();

private:
    // camAim有目标经过归一化后的输出值，以及对应的yaw和pitch值，是相机坐标系
    IMUAttitude imuAtt;
    CamTargetData camAim;

    BodyTarget bodyT;
    ControlErr controlE;

    FusionConfig _config;

    std::thread *pFusionThread = nullptr;

    /************************************* */
private:
    void fusionThread(void);

    BodyTarget cam2body(const CamTargetData cam);
    ControlErr body2world(const BodyTarget body, const IMUAttitude imu);
};
#endif // __MFUSION_HPP__