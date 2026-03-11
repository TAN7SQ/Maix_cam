# udp_mjpeg_viewer.py
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
    """UDP MJPEG Viewer for MaixCAM"""

    def __init__(self, port=5000, timeout=3, parent=None):
        super().__init__(parent)
        self.port = port
        self.timeout = timeout
        self.sock = None
        self.last_recv_time = 0
        self.is_running = False
        self.fps = 0
        self.frame_count = 0
        self.last_fps_time = time.time()
        self.has_received_frame = False  # 标记是否收到过画面
        self.window_shown = False  # 标记窗口是否已显示

        self.init_ui()
        # 初始隐藏窗口
        self.hide()
        self.start_receiver()

    def init_ui(self):
        self.setWindowTitle("UDP MJPEG Viewer")
        self.setMinimumSize(640, 480)
        self.main_layout = QVBoxLayout(self)
        self.main_layout.setContentsMargins(5, 5, 5, 5)

        self.status_label = QLabel("Status: Not Connected")
        self.status_label.setStyleSheet("color: #FF5722;")
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
        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)

        # UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("0.0.0.0", self.port))
        self.sock.settimeout(0.5)
        self.last_recv_time = time.time()

        # 定时器更新画面
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
        self.has_received_frame = False  # 重置画面接收标记

    @Slot()
    def receive_frame(self):
        if not self.is_running or not self.sock:
            return
        try:
            data, addr = self.sock.recvfrom(65536)
            self.last_recv_time = time.time()
            np_data = np.frombuffer(data, dtype=np.uint8)
            img = cv2.imdecode(np_data, cv2.IMREAD_COLOR)
            if img is not None:
                # 标记已收到画面
                self.has_received_frame = True
                
                # 如果窗口还没显示，自动显示
                if not self.window_shown:
                    self.show()
                    self.raise_()  # 提升窗口到前台
                    self.activateWindow()  # 激活窗口
                    self.window_shown = True

                # FPS计算
                self.frame_count += 1
                now = time.time()
                if now - self.last_fps_time >= 1.0:
                    self.fps = self.frame_count / (now - self.last_fps_time)
                    self.frame_count = 0
                    self.last_fps_time = now

                # 转为 Qt 格式
                h, w, ch = img.shape
                bytes_per_line = ch * w
                qimg = QImage(
                    img.data, w, h, bytes_per_line, QImage.Format.Format_BGR888
                )
                pixmap = QPixmap.fromImage(qimg)
                self.video_label.setPixmap(
                    pixmap.scaled(
                        self.video_label.size(),
                        Qt.KeepAspectRatio,
                        Qt.SmoothTransformation,
                    )
                )
                self.status_label.setText(f"Connected - FPS: {self.fps:.1f}")

        except socket.timeout:
            pass
        except Exception as e:
            self.status_label.setText(f"Error: {str(e)}")

        # 检查超时
        if time.time() - self.last_recv_time > self.timeout:
            self.status_label.setText("Device disconnected")
            self.stop_receiver()
            self.window_shown = False  # 超时后重置窗口显示标记
            self.hide()  # 超时后隐藏窗口


if __name__ == "__main__":
    # 可选：启动测试 UDP MJPEG 发送模拟
    import threading
    import random

    def test_udp_sender():
        """模拟 MJPEG 数据发送"""
        import cv2
        import socket

        sender_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        target = ("127.0.0.1", 5000)
        cap = cv2.VideoCapture(0)  # 使用电脑摄像头
        if not cap.isOpened():
            print("Camera not found, skipping test sender")
            return

        while True:
            ret, frame = cap.read()
            if not ret:
                continue
            # 加点文字
            cv2.putText(
                frame,
                f"Time: {time.time():.1f}",
                (50, 50),
                cv2.FONT_HERSHEY_SIMPLEX,
                1,
                (0, 0, 255),
                2,
            )
            # JPEG编码
            ret2, buf = cv2.imencode(".jpg", frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
            if ret2:
                sender_sock.sendto(buf.tobytes(), target)
            time.sleep(0.03)  # 约30 FPS

    # 取消注释下面这行可以测试自动显示功能
    # threading.Thread(target=test_udp_sender, daemon=True).start()

    # 启动 GUI
    app = QApplication(sys.argv)
    viewer = UDPMJPEGViewer(port=5000, timeout=3)
    # 不再主动show，改为收到画面后自动show
    sys.exit(app.exec())