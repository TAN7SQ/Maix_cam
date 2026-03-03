#ifndef __KALMAN_FILTER_HPP__
#define __KALMAN_FILTER_HPP__

#pragma once

#include <iostream>

class KalmanFilter
{
public:
    KalmanFilter() = default;
    ~KalmanFilter() = default;

    void predict();
    void update(float measurement);

    float getState() const { return state_; }

private:
    float state_;
    float covariance_;
    float processNoise_;
    float measurementNoise_;
};

void test_kalman(void);

#endif // __KALMAN_FILTER_HPP__