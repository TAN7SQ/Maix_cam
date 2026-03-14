import sys
import socket
import time
import struct
import av
import numpy as np
import cv2

from PySide6.QtWidgets import (
    QApplication,
    QWidget,
    QVBoxLayout,
    QLabel,
    QPushButton,
    QHBoxLayout,
)
from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QImage, QPixmap


HEADER_SIZE = 8


class UDPH264Viewer(QWidget):

    def __init__(self, port=5000, timeout=3):
        super().__init__()

        self.port = port
        self.timeout = timeout

        self.sock = None
        self.is_running = False

        self.codec = av.CodecContext.create("h264", "r")

        self.frames = {}

        self.last_recv_time = time.time()

        self.fps = 0
        self.frame_count = 0
        self.last_fps_time = time.time()

        self.start_time = None

        self.init_ui()

    def init_ui(self):

        self.setWindowTitle("UDP H264 Viewer")
        self.resize(800, 600)

        layout = QVBoxLayout(self)

        self.runtime_label = QLabel("Runtime: 00:00:00")
        layout.addWidget(self.runtime_label)

        self.status_label = QLabel("Status: Stopped")
        layout.addWidget(self.status_label)

        self.video_label = QLabel("Waiting for video...")
        self.video_label.setAlignment(Qt.AlignCenter)
        self.video_label.setStyleSheet("background:black;color:white")
        layout.addWidget(self.video_label, 1)

        btn_layout = QHBoxLayout()

        self.start_btn = QPushButton("Start")
        self.stop_btn = QPushButton("Stop")

        btn_layout.addWidget(self.start_btn)
        btn_layout.addWidget(self.stop_btn)

        layout.addLayout(btn_layout)

        self.start_btn.clicked.connect(self.start_receiver)
        self.stop_btn.clicked.connect(self.stop_receiver)

        self.stop_btn.setEnabled(False)

    def start_receiver(self):

        if self.is_running:
            return

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("0.0.0.0", self.port))
        self.sock.setblocking(False)

        self.is_running = True

        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)

        self.status_label.setText(f"Listening UDP {self.port}")

        self.timer = QTimer()
        self.timer.timeout.connect(self.receive_frame)
        self.timer.start(1)

        self.start_time = time.time()

    def stop_receiver(self):

        self.is_running = False

        if self.sock:
            self.sock.close()
            self.sock = None

        if hasattr(self, "timer"):
            self.timer.stop()

        self.start_btn.setEnabled(True)
        self.stop_btn.setEnabled(False)

        self.status_label.setText("Stopped")

    def update_runtime(self):

        if not self.start_time:
            return

        t = int(time.time() - self.start_time)

        h = t // 3600
        m = (t % 3600) // 60
        s = t % 60

        self.runtime_label.setText(f"Runtime: {h:02}:{m:02}:{s:02}")

    def receive_frame(self):

        self.update_runtime()

        if not self.sock:
            return

        try:

            data, _ = self.sock.recvfrom(1500)

            self.last_recv_time = time.time()

            if len(data) < HEADER_SIZE:
                return

            frame_id, packet_id, packet_total, payload_size = struct.unpack(
                "HHHH", data[:HEADER_SIZE]
            )

            payload = data[HEADER_SIZE : HEADER_SIZE + payload_size]

            if frame_id not in self.frames:
                self.frames[frame_id] = {
                    "packets": {},
                    "total": packet_total,
                    "time": time.time(),
                }

            self.frames[frame_id]["packets"][packet_id] = payload

            if len(self.frames[frame_id]["packets"]) == packet_total:

                packets = self.frames[frame_id]["packets"]

                frame_data = b"".join(packets[i] for i in range(packet_total))

                del self.frames[frame_id]

                self.decode_frame(frame_data)

        except BlockingIOError:
            pass

        except Exception as e:
            print(e)

        # 清理超时帧
        now = time.time()

        remove = []

        for fid, info in self.frames.items():
            if now - info["time"] > 1.0:
                remove.append(fid)

        for fid in remove:
            del self.frames[fid]

        if time.time() - self.last_recv_time > self.timeout:
            self.status_label.setText("Device disconnected")

    def decode_frame(self, data):

        try:

            packet = av.Packet(data)

            frames = self.codec.decode(packet)

            for frame in frames:

                img = frame.to_ndarray(format="bgr24")

                self.display_frame(img)

        except Exception:
            pass

    def display_frame(self, img):

        self.frame_count += 1

        now = time.time()

        if now - self.last_fps_time >= 1.0:

            self.fps = self.frame_count / (now - self.last_fps_time)

            self.frame_count = 0
            self.last_fps_time = now

        h, w, ch = img.shape
        bytes_per_line = ch * w

        qimg = QImage(
            img.data,
            w,
            h,
            bytes_per_line,
            QImage.Format_BGR888,
        )

        pixmap = QPixmap.fromImage(qimg)

        self.video_label.setPixmap(
            pixmap.scaled(
                self.video_label.size(),
                Qt.KeepAspectRatio,
                Qt.SmoothTransformation,
            )
        )

        self.status_label.setText(f"Connected | FPS: {self.fps:.1f}")


if __name__ == "__main__":

    app = QApplication(sys.argv)

    viewer = UDPH264Viewer(port=5000)

    viewer.show()

    sys.exit(app.exec())
