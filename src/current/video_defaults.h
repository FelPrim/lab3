#pragma once

// C++17 inline constants (safe to include)
inline constexpr int DEFAULT_BITRATE = 10000000; // 400 kb/s
inline constexpr int DEFAULT_FPS     = 60;
inline constexpr int DEFAULT_WIDTH   = 1920;
inline constexpr int DEFAULT_HEIGHT  = 1080;

// x264 presets: use "ultrafast" for lowest latency/CPU cost, and "zerolatency" tune
inline constexpr const char DEFAULT_X264_PRESET[] = "ultrafast";
inline constexpr const char DEFAULT_X264_TUNE[]   = "zerolatency";

// Subtract threads (leave one thread for decoder / system)
inline constexpr unsigned DEFAULT_HW_THREADS_SUBTRACT = 1;
