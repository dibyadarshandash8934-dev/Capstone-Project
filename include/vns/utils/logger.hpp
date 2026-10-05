#pragma once

#include <string>
#include <mutex>
#include <chrono>
#include <memory>
#include <cstdio>

namespace vns {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger& instance();
    
    void set_level(LogLevel level);
    void set_output_file(const std::string& path);
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
    template<typename... Args>
    void debug_fmt(const char* fmt, Args... args) {
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), fmt, args...);
        debug(buffer);
    }

    template<typename... Args>
    void info_fmt(const char* fmt, Args... args) {
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), fmt, args...);
        info(buffer);
    }

    template<typename... Args>
    void warning_fmt(const char* fmt, Args... args) {
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), fmt, args...);
        warning(buffer);
    }

    template<typename... Args>
    void error_fmt(const char* fmt, Args... args) {
        char buffer[1024];
        std::snprintf(buffer, sizeof(buffer), fmt, args...);
        error(buffer);
    }

private:
    Logger();
    ~Logger();
    
    void log(LogLevel level, const std::string& message);
    
    LogLevel level_;
    std::mutex mutex_;
    std::FILE* file_;
};

// Convenience macros
#define VNS_LOG_DEBUG(msg) vns::Logger::instance().debug(msg)
#define VNS_LOG_INFO(msg) vns::Logger::instance().info(msg)
#define VNS_LOG_WARN(msg) vns::Logger::instance().warning(msg)
#define VNS_LOG_ERROR(msg) vns::Logger::instance().error(msg)

#define VNS_LOG_DEBUG_FMT(...) vns::Logger::instance().debug_fmt(__VA_ARGS__)
#define VNS_LOG_INFO_FMT(...) vns::Logger::instance().info_fmt(__VA_ARGS__)
#define VNS_LOG_WARN_FMT(...) vns::Logger::instance().warning_fmt(__VA_ARGS__)
#define VNS_LOG_ERROR_FMT(...) vns::Logger::instance().error_fmt(__VA_ARGS__)

} // namespace vns