#pragma once
#include <cstdint>

// C++17 inline constants (safe to include)
inline constexpr int DEFAULT_BITRATE = 400000; // 400 kb/s
inline constexpr int DEFAULT_FPS     = 5;
inline constexpr int DEFAULT_WIDTH   = 640;
inline constexpr int DEFAULT_HEIGHT  = 480;
inline constexpr float DEFAULT_BUFFERSECONDS = 3;

// x264 presets: use "ultrafast" for lowest latency/CPU cost, and "zerolatency" tune
inline constexpr const char DEFAULT_X264_PRESET[] = "ultrafast";
inline constexpr const char DEFAULT_X264_TUNE[]   = "zerolatency";

// Subtract threads (leave one thread for decoder / system)
inline constexpr unsigned DEFAULT_HW_THREADS_SUBTRACT = 2;
inline constexpr const char* DEFAULT_ECHO_SERVER_ADDRESS = "95.81.125.224";
inline constexpr uint16_t DEFAULT_ECHO_SERVER_PORT = 23231;
inline constexpr uint16_t DEFAULT_UDP_CLIENT_PORT = 23233;
inline constexpr int MAX_PACKET_SIZE = 1200;

inline constexpr int MARGIN = 5;