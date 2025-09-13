#pragma once
#include <format>
#include <print>

// cant use LogLvl::ERROR cause of stupid Windows.h :(
enum class LogLvl{CRITICAL, ERR, WARN, INFO, DEBUG, TRACE};


template<>
struct std::formatter<LogLvl> : std::formatter<std::string_view> {
    auto format(LogLvl level, std::format_context& ctx) const {
        std::string_view str;
        switch (level) {
            case LogLvl::CRITICAL: str = "CRITICAL"; break;
            case LogLvl::ERR:      str = "ERROR"; break;
            case LogLvl::WARN:     str = "WARNING"; break;
            case LogLvl::INFO:     str = "INFO"; break;
            case LogLvl::DEBUG:    str = "DEBUG"; break;
            case LogLvl::TRACE:    str = "TRACE"; break;
            default:               str = "UNKNOWN"; break;
        }
        return std::formatter<std::string_view>::format(str, ctx);
    }
};

#define LOG(lvl, ...) \
    do { \
        if (LOGLVL >= LogLvl::lvl) { \
            if (LogLvl::lvl >= LogLvl::ERR) \
                std::println(stderr, __VA_ARGS__); \
            else \
                std::println(stdout, __VA_ARGS__); \
        } \
    } while (0)


