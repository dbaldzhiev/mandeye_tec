#include "logger.h"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>

namespace mandeye {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::SetLogFile(const std::string& path) {
    std::lock_guard<std::mutex> l(mutex_);
    path_ = path;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    file_.open(path_, std::ios::app);
}

void Logger::rotateIfNeeded() {
    if (path_.empty()) return;
    if (std::filesystem::exists(path_) && std::filesystem::file_size(path_) > max_size_) {
        file_.close();
        std::filesystem::path p(path_);
        std::filesystem::path rotated = p;
        rotated += ".1";
        std::error_code ec;
        std::filesystem::rename(p, rotated, ec);
        file_.open(path_, std::ios::trunc);
    }
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> l(mutex_);
    rotateIfNeeded();
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    const char* level_str = level == LogLevel::Info ? "INFO" : (level == LogLevel::Warn ? "WARN" : "ERROR");
    if (!file_.is_open()) {
        file_.open(path_, std::ios::app);
    }
    file_ << "[" << buf << "] [" << level_str << "] " << message << std::endl;
    std::cerr << "[" << level_str << "] " << message << std::endl;
}

} // namespace mandeye
