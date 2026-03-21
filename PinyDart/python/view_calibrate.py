import sys
import socket
import cv2
import numpy as np
import time

from PySide6.QtWidgets import (
    QApplication,
    QWidget,
    QVBoxLayout,
    QLabel,
    QPushButton,
    QHBoxLayout,
)
from PySide6.QtCore import Qt, QTimer, Slot
from PySide6.QtGui import QImage, QPixmap


class UDPMJPEGViewer(QWidget):
    def __init__(self, port=5000, timeout=3, parent=None):
        super().__init__(parent)

        self.port = port
        self.timeout = timeout

        self.sock = None
        self.is_running = False
        self.last_recv_time = 0

        # FPS
        self.fps = 0
        self.frame_count = 0
        self.last_fps_time = time.time()

        # Runtime
        self.start_time = None

        # ===== 标定参数（已修正）=====
        self.calib_mode = True
        # 【必须和你的棋盘格匹配！】这里按你截图的棋盘格设为(6,7)，如果实际不对请重新数
        self.pattern_size = (6, 8)  # 棋盘格内角点 (列, 行)
        self.square_size = 20.0  # 每个格子的实际大小，单位mm，按你打印的实际尺寸修改

        self.objpoints = []
        self.imgpoints = []

        self.max_samples = 60
        self.min_sample_interval = 1.0  # 采样间隔1秒，避免重复
        self.last_sample_time = 0

        # 生成棋盘格世界坐标
        self.objp = np.zeros(
            (self.pattern_size[0] * self.pattern_size[1], 3), np.float32
        )
        self.objp[:, :2] = (
            np.mgrid[0 : self.pattern_size[0], 0 : self.pattern_size[1]].T.reshape(
                -1, 2
            )
            * self.square_size
        )

        self.init_ui()
        self.start_receiver()

    def init_ui(self):
        self.setWindowTitle("UDP MJPEG Calibrator")
        self.setMinimumSize(640, 480)

        self.main_layout = QVBoxLayout(self)

        self.runtime_label = QLabel("Runtime: 00:00:00")
        self.main_layout.addWidget(self.runtime_label)

        self.status_label = QLabel("Status: Not Connected")
        self.main_layout.addWidget(self.status_label)

        self.video_label = QLabel("Waiting for video...")
        self.video_label.setAlignment(Qt.AlignCenter)
        self.video_label.setStyleSheet("background-color: black; color: white;")
        self.main_layout.addWidget(self.video_label, 1)

        self.btn_layout = QHBoxLayout()

        self.start_btn = QPushButton("Start")
        self.start_btn.clicked.connect(self.start_receiver)

        self.stop_btn = QPushButton("Stop")
        self.stop_btn.clicked.connect(self.stop_receiver)
        self.stop_btn.setEnabled(False)

        self.btn_layout.addWidget(self.start_btn)
        self.btn_layout.addWidget(self.stop_btn)

        self.main_layout.addLayout(self.btn_layout)

    def start_receiver(self):
        if self.is_running:
            return

        self.is_running = True
        # 重置标定状态
        self.calib_mode = True
        self.objpoints = []
        self.imgpoints = []
        self.last_sample_time = 0

        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("0.0.0.0", self.port))
        self.sock.settimeout(0.5)

        self.last_recv_time = time.time()
        self.start_time = time.time()

        self.timer = QTimer()
        self.timer.timeout.connect(self.receive_frame)
        self.timer.start(10)

        self.status_label.setText(f"Listening on UDP port {self.port}")

    def stop_receiver(self):
        self.is_running = False

        self.start_btn.setEnabled(True)
        self.stop_btn.setEnabled(False)

        self.status_label.setText("Stopped")

        if hasattr(self, "timer"):
            self.timer.stop()

        if self.sock:
            self.sock.close()
            self.sock = None

        self.video_label.setText("No video")

    def update_runtime(self):
        if self.start_time is None:
            return

        elapsed = int(time.time() - self.start_time)

        h = elapsed // 3600
        m = (elapsed % 3600) // 60
        s = elapsed % 60

        self.runtime_label.setText(f"Runtime: {h:02d}:{m:02d}:{s:02d}")

    @Slot()
    def receive_frame(self):

        self.update_runtime()

        if not self.is_running or not self.sock:
            return

        try:
            data, _ = self.sock.recvfrom(65536)

            self.last_recv_time = time.time()

            np_data = np.frombuffer(data, dtype=np.uint8)
            img = cv2.imdecode(np_data, cv2.IMREAD_COLOR)

            if img is not None:
                # 先转灰度图，提升角点检测成功率
                gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                # 直方图均衡化，解决光照不均的问题
                gray = cv2.equalizeHist(gray)

                # ===== 标定逻辑（已修复）=====
                if self.calib_mode:
                    # 查找棋盘格角点，增加标志提升检测成功率
                    ret, corners = cv2.findChessboardCorners(
                        gray,
                        self.pattern_size,
                        cv2.CALIB_CB_ADAPTIVE_THRESH
                        + cv2.CALIB_CB_NORMALIZE_IMAGE
                        + cv2.CALIB_CB_FAST_CHECK,
                    )

                    if ret:
                        # 亚像素优化角点
                        corners2 = cv2.cornerSubPix(
                            gray,
                            corners,
                            (11, 11),
                            (-1, -1),
                            (
                                cv2.TermCriteria_EPS + cv2.TermCriteria_MAX_ITER,
                                30,
                                0.001,
                            ),
                        )

                        # 绘制角点，方便观察
                        cv2.drawChessboardCorners(img, self.pattern_size, corners2, ret)

                        # 采样控制：间隔1秒，避免重复样本
                        if (
                            time.time() - self.last_sample_time
                            > self.min_sample_interval
                        ):
                            self.objpoints.append(self.objp)
                            self.imgpoints.append(corners2)
                            self.last_sample_time = time.time()
                            print(
                                f"[INFO] 已采集样本: {len(self.objpoints)}/{self.max_samples}"
                            )

                    # 达到样本数量，开始标定
                    if len(self.objpoints) >= self.max_samples:
                        print("[INFO] 开始标定计算...")
                        # 执行标定
                        ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(
                            self.objpoints,
                            self.imgpoints,
                            gray.shape[::-1],
                            None,
                            None,
                        )

                        if ret:
                            print("\n===== 标定成功！结果 =====")
                            print("相机内参矩阵 Camera Matrix:\n", mtx)
                            print("\n畸变系数 Distortion Coefficients:\n", dist)
                            # 计算重投影误差，判断标定质量
                            mean_error = 0
                            for i in range(len(self.objpoints)):
                                imgpoints2, _ = cv2.projectPoints(
                                    self.objpoints[i], rvecs[i], tvecs[i], mtx, dist
                                )
                                error = cv2.norm(
                                    self.imgpoints[i], imgpoints2, cv2.NORM_L2
                                ) / len(imgpoints2)
                                mean_error += error
                            mean_error /= len(self.objpoints)
                            print(
                                f"\n平均重投影误差: {mean_error:.4f} 像素（小于0.5为优秀，小于1.0为合格）"
                            )

                            # 保存参数
                            np.savez(
                                "camera_calib.npz",
                                mtx=mtx,
                                dist=dist,
                                pattern_size=self.pattern_size,
                                square_size=self.square_size,
                            )
                            print("[INFO] 参数已保存到 camera_calib.npz")
                            self.status_label.setText(
                                f"标定完成！误差: {mean_error:.4f} 像素"
                            )
                        else:
                            print("[ERROR] 标定失败，请检查样本质量")
                            self.status_label.setText("标定失败，请重新采集样本")

                        self.calib_mode = False

                # FPS统计
                self.frame_count += 1
                now = time.time()
                if now - self.last_fps_time >= 1.0:
                    self.fps = self.frame_count / (now - self.last_fps_time)
                    self.frame_count = 0
                    self.last_fps_time = now

                # 画面显示
                h, w, ch = img.shape
                bytes_per_line = ch * w
                qimg = QImage(img.data, w, h, bytes_per_line, QImage.Format_BGR888)
                pixmap = QPixmap.fromImage(qimg)
                self.video_label.setPixmap(
                    pixmap.scaled(
                        self.video_label.size(),
                        Qt.KeepAspectRatio,
                        Qt.SmoothTransformation,
                    )
                )

                # 状态更新
                if self.calib_mode:
                    self.status_label.setText(
                        f"标定中... 已采集 {len(self.objpoints)}/{self.max_samples} 帧 | FPS: {self.fps:.1f}"
                    )

        except socket.timeout:
            pass
        except Exception as e:
            self.status_label.setText(f"Error: {str(e)}")
            print(f"[ERROR] {str(e)}")

        # 超时检测
        if time.time() - self.last_recv_time > self.timeout:
            self.status_label.setText("设备断开连接")
            self.stop_receiver()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    # 【注意】端口要和你的飞镖发送端一致！
    viewer = UDPMJPEGViewer(port=5000, timeout=3)
    viewer.show()
    sys.exit(app.exec())
