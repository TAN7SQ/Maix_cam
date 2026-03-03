// 状态向量：[q0, q1, q2, q3, bgx, bgy, bgz]
float state[7];

// 状态协方差矩阵 P (7x7)
float P[49];

// 过程噪声协方差矩阵 Q (7x7)
float Q[49];

// 观测噪声协方差矩阵 R (6x6)：3个加速度计和3个磁力计
float R[36];

// 状态转换矩阵 F (7x7)
float F[49];

// 观测矩阵 H (6x7)
float H[42];

// 卡尔曼增益 K (7x6)
float K[42];

// 预测状态
float state_pred[7];

// 预测协方差
float P_pred[49];

// 残差
float y[6];

// S = H*P*H^T + R
float S[36];

// 初始状态
void init_state(float initial_q[4], float initial_bg[3])
{
    // 初始化状态向量
    memcpy(state, initial_q, 4 * sizeof(float));
    memcpy(state + 4, initial_bg, 3 * sizeof(float));

    // 初始化协方差矩阵 P
    memset(P, 0, 49 * sizeof(float));
    P[0] = 0.01f;
    P[7] = 0.01f;
    P[14] = 0.01f;
    P[21] = 0.01f; // q的方差
    P[28] = 0.01f;
    P[35] = 0.01f;
    P[42] = 0.01f; // bg的方差

    // 初始化过程噪声协方差 Q
    memset(Q, 0, 49 * sizeof(float));
    Q[0] = 0.001f;
    Q[7] = 0.001f;
    Q[14] = 0.001f;
    Q[21] = 0.001f; // q的噪声
    Q[28] = 0.0001f;
    Q[35] = 0.0001f;
    Q[42] = 0.0001f; // bg的噪声

    // 初始化观测噪声协方差 R
    memset(R, 0, 36 * sizeof(float));
    R[0] = 0.1f;
    R[7] = 0.1f;
    R[14] = 0.1f; // 加速度计噪声
    R[21] = 0.1f;
    R[28] = 0.1f;
    R[35] = 0.1f; // 磁力计噪声
}

// 预测步骤
void predict(float gyro[3], float dt)
{
    // 角速度纯四元数 omega = [0, wx, wy, wz]
    float omega[4] = {0, gyro[0], gyro[1], gyro[2]};

    // 离散时间内的旋转角 theta = ||omega|| * dt
    float theta = sqrt(omega[1] * omega[1] + omega[2] * omega[2] + omega[3] * omega[3]) * dt;

    // 旋转轴的单位向量 axis = omega/theta (归一化)
    float axis[3];
    if (theta > 1e-6) {
        axis[0] = omega[1] / theta;
        axis[1] = omega[2] / theta;
        axis[2] = omega[3] / theta;
    }
    else {
        axis[0] = 0;
        axis[1] = 0;
        axis[2] = 0;
    }

    // 离散旋转对应的增量四元数 dq = [cos(theta/2), axis[0]*sin(theta/2), axis[1]*sin(theta/2), axis[2]*sin(theta/2)]
    float dq[4] = {cos(theta / 2), axis[0] * sin(theta / 2), axis[1] * sin(theta / 2), axis[2] * sin(theta / 2)};

    // 预测四元数 q = state * dq (四元数乘法)
    float q[4];
    quaternion_multiply(state, dq, q);
    // 归一化四元数 q
    quaternion_normalize(q);
    // 更新预测状态 state_pred = q
    memcpy(state_pred, q, 4 * sizeof(float));

    // 陀螺仪偏置保持不变
    // 陀螺仪偏置的变化速度远慢于姿态变化速度（比如偏置可能几秒甚至几分钟才变化一点），因此在短时间步
    // dt内，可以认为偏置是恒定的。后续更新步骤会根据观测数据，缓慢修正这个偏置值，实现零漂校准。
    memcpy(state_pred + 4, state + 4, 3 * sizeof(float));

    // 计算状态转换矩阵 F 这里简化为恒等矩阵 I（省略了过程噪声输入矩阵 G 的影响）
    // (这里简化为恒等矩阵，实际应用中需要正确计算)
    memset(F, 0, 49 * sizeof(float));
    for (int i = 0; i < 7; i++)
        F[i * 7 + i] = 1.0f;

    // P_pred = F*P*F^T + G*Q*G^T
    // (这里简化为P_pred = P + Q)
    for (int i = 0; i < 49; i++)
        P_pred[i] = P[i] + Q[i];

    // 更新状态和协方差
    memcpy(state, state_pred, 7 * sizeof(float));
    memcpy(P, P_pred, 49 * sizeof(float));
}

// 更新步骤，观测值为加速度计和磁力计测量值，修正使用陀螺仪预测步骤得到的先验状态
/*
acc: 加速度计测量值数组 [ax, ay, az]
mag: 磁力计测量值数组 [mx, my, mz]
ref_mag: 参考磁场向量数组 [mx_ref, my_ref, mz_ref ] (地磁场参考值，用于转换得到理想磁力计观测值)
state: 先验状态向量 [q0, q1, q2, q3, bgx, bgy, bgz]
P_pred: 先验协方差矩阵 (7x7)
R: 观测噪声协方差矩阵 (6x6)
P: 后验协方差矩阵 (7x7)

1. 计算预测值
2. 计算残差
3. 计算残差协方差
4. 计算卡尔曼增益
5. 修正状态与协方差
*/
void update(float acc[3], float mag[3], float ref_mag[3])
{

    // 计算期望的加速度计测量值
    float q_inv[4];
    // 计算先验姿态四元数q的共轭四元数 q_inv
    quaternion_conjugate(state, q_inv);

    // 重力向量在导航坐标系下为 [0, 0, 0, -1] <== [0,0,-g]
    float expected_acc[4] = {0, 0, 0, -1};
    // 通过四元数变换计算在机体坐标系下的重力向量 expected_acc = q * [0, 0, 0, -1] * q_inv
    quaternion_multiply(state, expected_acc, expected_acc);
    quaternion_multiply(expected_acc, q_inv, expected_acc);
    // 归一化重力向量
    expected_acc[1] /= expected_acc[0];
    expected_acc[2] /= expected_acc[0];
    expected_acc[3] /= expected_acc[0];

    // 计算期望的磁力计测量值
    float expected_mag[4];
    // 通过四元数变换计算在机体坐标系下的参考磁场向量 expected_mag = q * ref_mag * q_inv
    quaternion_multiply(state, ref_mag, expected_mag);
    quaternion_multiply(expected_mag, q_inv, expected_mag);
    // 归一化磁力计向量
    expected_mag[1] /= expected_mag[0];
    expected_mag[2] /= expected_mag[0];
    expected_mag[3] /= expected_mag[0];

    // 组合实际的观测向量
    float z[6] = {
        acc[0],
        acc[1],
        acc[2], // 实际加速度计测量值
        mag[0],
        mag[1],
        mag[2] // 实际磁力计测量值
    };

    float h[6] = {
        expected_acc[1],
        expected_acc[2],
        expected_acc[3], // 预测的加速度计测量值
        expected_mag[1],
        expected_mag[2],
        expected_mag[3] // 预测的磁力计测量值
    };

    // 计算残差 y = z - h
    // 若残差y为0，说明先验状态非常准确，几乎不需要修正
    // 若残差y较大，说明先验状态偏离真实状态较多，需要通过观测值进行较大修正，
    // ① 先验状态误差大（陀螺仪推算偏差）；
    // ② 传感器观测噪声大（加速度计受运动加速度干扰、磁力计受磁场干扰）
    memset(y, 0, 6 * sizeof(float));
    for (int i = 0; i < 6; i++)
        y[i] = z[i] - h[i];

    // 计算观测矩阵 H
    // (这里简化为恒等矩阵，实际应用中需要正确计算)
    memset(H, 0, 42 * sizeof(float));
    for (int i = 0; i < 6; i++)
        H[i * 8 + i] = 1.0f;

    // 计算 S = H*P*H^T + R
    // (这里简化为 S = H*P*H^T + R)
    memset(S, 0, 36 * sizeof(float));
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 7; j++) {
            if (H[i * 7 + j] == 0)
                continue;
            for (int k = 0; k < 6; k++) {
                if (H[k * 7 + j] == 0)
                    continue;
                S[i * 6 + k] += H[i * 7 + j] * P[j * 7 + k] * H[k * 7 + j];
            }
        }
    }
    for (int i = 0; i < 36; i++)
        S[i] += R[i];

    // 计算卡尔曼增益 K = P*H^T*S^{-1}
    // (这里简化为 K = P*H^T / (H*P*H^T + R))
    memset(K, 0, 42 * sizeof(float));
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 6; j++) {
            float sum = 0;
            for (int k = 0; k < 7; k++) {
                sum += P[i * 7 + k] * H[j * 7 + k];
            }
            K[i * 6 + j] = sum / S[j * 6 + j];
        }
    }

    // 更新状态 state = state + K*y
    for (int i = 0; i < 7; i++) {
        float sum = 0;
        for (int j = 0; j < 6; j++) {
            sum += K[i * 6 + j] * y[j];
        }
        // 更新状态向量 state[i] = state[i] + K[i*6]*y[j]
        // 最优状态 = 先验状态 + 卡尔曼增益 * 残差 = 先验状态 + 修正量
        state[i] += sum;
    }

    // 更新协方差 P = (I - K*H)*P_pred （简化版，工程上最常用）
    memset(P, 0, 49 * sizeof(float));
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            float sum = 0;
            for (int k = 0; k < 6; k++) {
                sum += K[i * 6 + k] * H[k * 7 + j];
            }
            P[i * 7 + j] = (1 - sum) * P_pred[i * 7 + j];
        }
    }
}