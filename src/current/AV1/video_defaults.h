// video_defaults.h
#pragma once


inline constexpr int DEFAULT_BITRATE = 400000; // 400 kb/s
inline constexpr int DEFAULT_FPS     = 15;
inline constexpr int DEFAULT_WIDTH   = 640;
inline constexpr int DEFAULT_HEIGHT  = 480;

// SVT preset: use inline array to avoid pointer multiple-definition headaches
// "8" — fast preset in some libsvtav1 builds (check your build).
inline constexpr const char DEFAULT_SVT_PRESET[] = "8";

// Hardware threads subtract
inline constexpr unsigned DEFAULT_HW_THREADS_SUBTRACT = 1;
