#pragma once
#include <cstdint>

// C++17 inline constants (safe to include)
inline constexpr int BITRATE_1080P_60FPS = 12000000;  
inline constexpr int BITRATE_1080P_30FPS = 8000000;  
inline constexpr int BITRATE_1080P_15FPS = 4800000; 
inline constexpr int BITRATE_720P_60FPS  = 6000000;   
inline constexpr int BITRATE_720P_30FPS  = 4000000; 
inline constexpr int BITRATE_720P_15FPS  = 2400000; 
inline constexpr int BITRATE_480P_60FPS  = 1500000; 
inline constexpr int BITRATE_480P_30FPS  = 1000000; 
inline constexpr int BITRATE_480P_15FPS  = 600000;   

inline constexpr int _1080P_WIDTH   = 1920;
inline constexpr int _1080P_HEIGHT  = 1080;
inline constexpr int _720P_WIDTH   = 1280;
inline constexpr int _720P_HEIGHT  = 720;
inline constexpr int _480P_WIDTH   = 854;
inline constexpr int _480P_HEIGHT  = 480;

inline constexpr int DEFAULT_BITRATE = BITRATE_480P_15FPS;
inline constexpr int DEFAULT_FPS     = 15;
inline constexpr int DEFAULT_WIDTH   = _480P_WIDTH;
inline constexpr int DEFAULT_HEIGHT  = _480P_HEIGHT;
inline constexpr float DEFAULT_BUFFERSECONDS = 0.1;

// x264 presets: use "ultrafast" for lowest latency/CPU cost, and "zerolatency" tune
inline constexpr const char DEFAULT_X264_PRESET[] = "ultrafast";
inline constexpr const char DEFAULT_X264_TUNE[]   = "zerolatency";

// Subtract threads (leave one thread for decoder / system)
inline constexpr unsigned DEFAULT_HW_THREADS_SUBTRACT = 2;
inline constexpr const char* DEFAULT_ECHO_SERVER_ADDRESS = "95.81.125.224";
inline constexpr uint16_t DEFAULT_ECHO_SERVER_PORT = 23231;
inline constexpr uint16_t DEFAULT_UDP_CLIENT_PORT = 23233;
inline constexpr int MAX_PACKET_SIZE = 1200;
inline constexpr int DEFAULT_BUFFERSZ = 128;
inline constexpr int MARGIN = 5;

// Добавляем константы для нового протокола
inline constexpr int PACKET_HEADER_SIZE = 16;
inline constexpr int MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - PACKET_HEADER_SIZE;
inline constexpr int FRAME_HEADER_SIZE = 8;

// FEC параметры для XOR-схемы
inline constexpr int FEC_GROUP_SIZE = 4;
inline constexpr int FEC_XOR_PACKETS = 5;
// FEC packet types
inline constexpr quint8 XOR12_PACKET = 0x05;
inline constexpr quint8 XOR23_PACKET = 0x06;
inline constexpr quint8 XOR34_PACKET = 0x07;
inline constexpr quint8 XOR14_PACKET = 0x08;
inline constexpr quint8 XOR1234_PACKET = 0x09;
