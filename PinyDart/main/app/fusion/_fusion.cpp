#include "main.h"
#include "maix_basic.hpp"

#include "_fusion.hpp"
#include "_vision.hpp"

namespace InstallErr
{
constexpr float CAMERA_PITCH_DEG = -20.0f; // 向下为负，向上为正
constexpr float CAMERA_YAW_DEG = 0.0f;     // 向右偏为正，向左偏为负
constexpr float CAMERA_ROLL_DEG = 0.0f;    // 顺时针滚为正，逆时针为负

constexpr float DEG2RAD = M_PI / 180.0f;

constexpr float PITCH_RAD = CAMERA_PITCH_DEG * DEG2RAD;
constexpr float YAW_RAD = CAMERA_YAW_DEG * DEG2RAD;
constexpr float ROLL_RAD = CAMERA_ROLL_DEG * DEG2RAD;

float CP = std::cos(PITCH_RAD);
float SP = std::sin(PITCH_RAD);
float CY = std::cos(YAW_RAD);
float SY = std::sin(YAW_RAD);
float CR = std::cos(ROLL_RAD);
float SR = std::sin(ROLL_RAD);

constexpr float TX = 0.0f; // 左右偏移
constexpr float TY = 0.0f; // 上下偏移
constexpr float TZ = 0.0f; // 前后偏移
} // namespace InstallErr

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

        maix::thread::sleep_ms(100);
        if (!Shared::gTargetQueue.empty())
            camAim = Shared::gTargetQueue.pop_non_blocking();

        if (!Shared::gImuAttitude.empty()) {
            imuAtt = Shared::gImuAttitude.pop_non_blocking();
            Log::trace(TAG, "%.2f,%.2f,%.2f,%.2f", imuAtt.quat.w, imuAtt.quat.x, imuAtt.quat.y, imuAtt.quat.z);
        }

        // bodyT = cam2body(camAim);
        // Log::info(TAG, "yaw:%.2f,pitch:%.2f", bodyT.yaw_error, bodyT.pitch_error);
        // controlE = body2world(bodyT, imuAtt);
        // Log::info(TAG, "yaw:%.2f,pitch:%.2f", controlE.yaw, controlE.pitch);
    }

    log::info("fusion thread exit");
}

BodyTarget Fusion::cam2body(const CamTargetData cam)
{
    using namespace InstallErr;

    BodyTarget result;
    result.valid = true;

    float x_c = cam.normX;
    float y_c = -cam.normY; // cam与body的坐标定义相反
    float z_c = 1.0f;

    // Yaw-Pitch-Roll

    float x1 = x_c * CY - y_c * SY;
    float y1 = x_c * SY + y_c * CY;
    float z1 = z_c;

    float x2 = x1 * CP + z1 * SP;
    float y2 = y1;
    float z2 = -x1 * SP + z1 * CP;

    float x3 = x2;
    float y3 = y2 * CR - z2 * SR;
    float z3 = y2 * SR + z2 * CR;

    // 平移补偿
    result.x = x3 + TX;
    result.y = y3 + TY;
    result.z = z3 + TZ;

    result.yaw_error = atan2(result.x, result.z);
    result.pitch_error = atan2(result.y, result.z);
    result.roll_error = atan2(result.y, sqrt(result.x * result.x + result.z * result.z));

    result.valid = true;

    return result;
}

ControlErr Fusion::body2world(const BodyTarget body, const IMUAttitude imu)
{
    ControlErr control;

    Vec3 bodyVec(body.x, body.y, body.z);
    Vec3 world = Tools::quat_rotate(imu.quat, bodyVec);

    control.yaw = std::atan2(world.x, world.z);
    control.pitch = std::atan2(-world.x, std::sqrt(world.z * world.z + world.x * world.x));
    return control;
}
