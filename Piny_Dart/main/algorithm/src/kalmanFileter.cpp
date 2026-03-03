#include "kalmanFilter.hpp"

#include <iostream>

void KalmanFilter::predict()
{
    // 预测步骤的简单实现
    state_ = state_; // 状态不变
    covariance_ += processNoise_;
}

void KalmanFilter::update(float measurement)
{
    // 更新步骤的简单实现
    float kalmanGain = covariance_ / (covariance_ + measurementNoise_);
    state_ = state_ + kalmanGain * (measurement - state_);
    covariance_ = (1 - kalmanGain) * covariance_;
}

void test_kalman(void)
{
    std::cout << "Kalman Filter Test" << std::endl;
}
