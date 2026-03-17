#pragma once
#include "_basic.hpp"
#include "nlohmann/json.hpp"

struct VisionConfig
{
    struct
    {
        std::vector<std::vector<int>> green_thresholds;
        int x_stride;
        int y_stride;
        int area_threshold;
        int pixels_threshold;
        bool merge;
        int margin;
        int x_hist_bins_max;
        int y_hist_bins_max;
    } find_blobs;

    struct
    {
        bool is_enabled;
        std::string udp_ip;
        int udp_port;
    } udp;

    struct
    {
        bool is_enabled;
        int fps;
        int bitrate;
        std::string mp4_path;
    } mp4;
};

struct ControlConfig
{
    float kp;
    float ki;
    float kd;
    float max_speed;
};

struct NetworkConfig
{
    std::string wifi_ssid;
    std::string wifi_password;
};

struct AppConfig
{
    VisionConfig vision;
    // ControlConfig control;
    // NetworkConfig network;
};

class ConfigJson
{
public:
    static constexpr const char *TAG = "json";

    static bool load(const std::string &filename, AppConfig &config);

    static void print_vision(const VisionConfig &config);
};