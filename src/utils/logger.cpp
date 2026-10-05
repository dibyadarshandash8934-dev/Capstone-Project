#include "vns/utils/logger.hpp"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <chrono>
#include <mutex>
#include <cstring>

namespace vns {

Logger::Logger() : level_(LogLevel::INFO), file_(nullptr) {}

Logger::~Logger() {
    if (file_) {
        std::fclose(file_);
        file_ = nullptr;
    }
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::set_output_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_) {
        std::fclose(file_);
        file_ = nullptr;
    }
    file_ = std::fopen(path.c_str(), "a");
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()) % 1000;
    
    char time_buf[32];
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
    
    const char* level_str;
    switch (level) {
        case LogLevel::DEBUG: level_str = "DEBUG"; break;
        case LogLevel::INFO: level_str = "INFO"; break;
        case LogLevel::WARNING: level_str = "WARN"; break;
        case LogLevel::ERROR: level_str = "ERROR"; break;
    }
    
    char output[512];
    std::snprintf(output, sizeof(output), "[%s.%03d] [%s] %s", 
                  time_buf, static_cast<int>(ms.count()), level_str, message.c_str());
    
    // Console output
    std::fprintf(stderr, "%s\n", output);
    
    // File output
    if (file_) {
        std::fprintf(file_, "%s\n", output);
        std::fflush(file_);
    }
}

void Logger::debug(const std::string& message) { log(LogLevel::DEBUG, message); }
void Logger::info(const std::string& message) { log(LogLevel::INFO, message); }
void Logger::warning(const std::string& message) { log(LogLevel::WARNING, message); }
void Logger::error(const std::string& message) { log(LogLevel::ERROR, message); }

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

} // namespace vns