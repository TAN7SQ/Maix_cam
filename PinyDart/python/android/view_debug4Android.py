import sys
import socket
import cv2
import numpy as np
import time
from threading import Thread
from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.uix.button import Button
from kivy.graphics.texture import Texture
from kivy.clock import Clock, mainthread
from kivy.core.window import Window
from kivy.config import Config

# 配置窗口大小（安卓上会自适应）
Config.set("graphics", "width", "640")
Config.set("graphics", "height", "480")
Config.set("graphics", "resizable", True)


class UDPMJPEGViewer(BoxLayout):
    """UDP MJPEG Viewer (安卓兼容版)"""

    def __init__(self, port=5000, timeout=3, **kwargs):
        super().__init__(**kwargs)
        self.orientation = "vertical"
        self.spacing = 5
        self.padding = 5

        self.port = port
        self.timeout = timeout
        self.sock = None
        self.is_running = False
        self.last_recv_time = 0
        self.fps = 0
        self.frame_count = 0
        self.last_fps_time = time.time()
        self.start_time = None
        self.receive_thread = None

        # UI 组件
        # 运行时间标签
        self.runtime_label = Label(
            text="Runtime: 00:00:00",
            color=(0.298, 0.686, 0.309, 1),  # #4CAF50 对应的 RGBA
            font_size="14sp",
            bold=True,
        )
        self.add_widget(self.runtime_label)

        # 状态标签
        self.status_label = Label(
            text="Status: Not Connected",
            color=(1.0, 0.341, 0.133, 1),  # #FF5722 对应的 RGBA
            font_size="14sp",
        )
        self.add_widget(self.status_label)

        # 视频显示标签
        self.video_label = Label(text="Waiting for video...")
        self.video_label.bind(size=self._resize_video)
        self.add_widget(self.video_label, weight=1)

        # 按钮布局
        btn_layout = BoxLayout(orientation="horizontal", size_hint_y=0.1)
        self.start_btn = Button(text="Start", on_press=self.start_receiver)
        self.stop_btn = Button(text="Stop", on_press=self.stop_receiver, disabled=True)
        btn_layout.add_widget(self.start_btn)
        btn_layout.add_widget(self.stop_btn)
        self.add_widget(btn_layout)

        # 定时更新运行时间
        Clock.schedule_interval(self.update_runtime, 1)

    def update_runtime(self, dt):
        """更新运行时间"""
        if self.start_time is None:
            return
        elapsed = int(time.time() - self.start_time)
        hours = elapsed // 3600
        minutes = (elapsed % 3600) // 60
        seconds = elapsed % 60
        self.runtime_label.text = f"Runtime: {hours:02d}:{minutes:02d}:{seconds:02d}"

    def start_receiver(self, instance):
        """启动UDP接收"""
        if self.is_running:
            return
        self.is_running = True
        self.start_btn.disabled = True
        self.stop_btn.disabled = False

        # 创建UDP套接字
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("0.0.0.0", self.port))
        self.sock.settimeout(0.5)

        self.last_recv_time = time.time()
        self.start_time = time.time()

        # 启动接收线程（避免阻塞UI）
        self.receive_thread = Thread(target=self.receive_frame, daemon=True)
        self.receive_thread.start()

        self.status_label.text = f"Listening on UDP port {self.port}"

    def stop_receiver(self, instance):
        """停止UDP接收"""
        self.is_running = False
        self.start_btn.disabled = False
        self.stop_btn.disabled = True
        self.status_label.text = "Stopped"

        if self.sock:
            self.sock.close()
            self.sock = None

        self.video_label.text = "No video"
        # 清空视频纹理
        self.video_label.texture = None

    @mainthread
    def update_video_frame(self, img):
        """在主线程更新视频帧（Kivy要求）"""
        # OpenCV图像转Kivy纹理
        buf = cv2.flip(img, 0).tobytes()
        texture = Texture.create(size=(img.shape[1], img.shape[0]), colorfmt="bgr")
        texture.blit_buffer(buf, colorfmt="bgr", bufferfmt="ubyte")
        self.video_label.texture = texture
        self.video_label.text = ""  # 清空占位文字

    @mainthread
    def update_status(self, text):
        """在主线程更新状态"""
        self.status_label.text = text

    def receive_frame(self):
        """UDP接收帧（后台线程）"""
        while self.is_running and self.sock:
            try:
                data, addr = self.sock.recvfrom(65536)
                self.last_recv_time = time.time()

                # 解码MJPEG数据
                np_data = np.frombuffer(data, dtype=np.uint8)
                img = cv2.imdecode(np_data, cv2.IMREAD_COLOR)

                if img is not None:
                    # 计算FPS
                    self.frame_count += 1
                    now = time.time()
                    if now - self.last_fps_time >= 1.0:
                        self.fps = self.frame_count / (now - self.last_fps_time)
                        self.frame_count = 0
                        self.last_fps_time = now
                        self.update_status(f"Connected - FPS: {self.fps:.1f}")

                    # 更新视频帧
                    self.update_video_frame(img)

            except socket.timeout:
                # 超时检测
                if time.time() - self.last_recv_time > self.timeout:
                    self.update_status("Device disconnected")
                    self.stop_receiver(None)
            except Exception as e:
                self.update_status(f"Error: {str(e)}")

    def _resize_video(self, instance, size):
        """视频帧自适应大小"""
        if self.video_label.texture:
            self.video_label.texture_size = size


class UDPMJPEGViewerApp(App):
    def build(self):
        self.title = "UDP MJPEG Viewer"
        return UDPMJPEGViewer(port=5000, timeout=3)


if __name__ == "__main__":
    # 测试用：安卓上注释这部分
    # import threading
    # def test_udp_sender():
    #     sender_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    #     target = ("127.0.0.1", 5000)
    #     cap = cv2.VideoCapture(0)
    #     if not cap.isOpened():
    #         print("Camera not found")
    #         return
    #     while True:
    #         ret, frame = cap.read()
    #         if not ret:
    #             continue
    #         cv2.putText(frame, f"Time: {time.time():.1f}", (50, 50),
    #                     cv2.FONT_HERSHEY_SIMPLEX, 1, (0,0,255), 2)
    #         ret2, buf = cv2.imencode(".jpg", frame, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
    #         if ret2:
    #             sender_sock.sendto(buf.tobytes(), target)
    #         time.sleep(0.03)
    # threading.Thread(target=test_udp_sender, daemon=True).start()

    UDPMJPEGViewerApp().run()
