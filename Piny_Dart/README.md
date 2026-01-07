# Piny_dart Project based on MaixCDK

# 1. Project Introduction

```mermaid
graph TD
    A[ESP32 Sensor Data] --> B[[Uart receive epoll thread]]
    B -->|thread safe| C[Sensor data queue ]
    C --> D[[Kalman filter data fusion thread]]
    D -->|thread safe| E[Filtered data queue]
    F[Camera/Video stream] --> G[CV processing]
    G --> H[[Display/Output RTSP stream thread]]
    G --> I[Real-time data visualization]
    I --> J[[Servo control calculate thread]]
    E --> J
    J --> K[Control data queue]
    K --> L[[Serial send data thread]]
    L --> M[ESP32 Servo Control Data]
    E --> N[[WebServer thread]]
    N --> O[WebServer PC Terminal]
```

# 2. Thread Introduction

1. Uart receive epoll thread:
2. Kalman filter data fusion thread:
3. Display/Output RTSP stream thread:
4. Servo control calculate thread:
5. Serial send data thread:
6. WebServer thread:
7. Real-time data visualization(CV):
