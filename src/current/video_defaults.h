#pragma once

// C++17 inline constants (safe to include)
inline constexpr int DEFAULT_BITRATE = 4000000; // 400 kb/s
inline constexpr int DEFAULT_FPS     = 15;
inline constexpr int DEFAULT_WIDTH   = 640;
inline constexpr int DEFAULT_HEIGHT  = 480;
inline constexpr float DEFAULT_BUFFERSECONDS = 0.1;

// x264 presets: use "ultrafast" for lowest latency/CPU cost, and "zerolatency" tune
inline constexpr const char DEFAULT_X264_PRESET[] = "ultrafast";
inline constexpr const char DEFAULT_X264_TUNE[]   = "zerolatency";

// Subtract threads (leave one thread for decoder / system)
inline constexpr unsigned DEFAULT_HW_THREADS_SUBTRACT = 1;
