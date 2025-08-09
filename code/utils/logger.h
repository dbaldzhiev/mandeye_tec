#pragma once
#include <fstream>
#include <mutex>
#include <string>

namespace mandeye {

enum class LogLevel { Info, Warn, Error };

class Logger {
 public:
  static Logger& Instance();
  void SetLogFile(const std::string& path);
  void Log(LogLevel level, const std::string& message);

 private:
  Logger() = default;
  void rotateIfNeeded();

  std::mutex mutex_;
  std::ofstream file_;
  std::string path_;
  const std::size_t max_size_ = 5 * 1024 * 1024; // 5 MB
};

} // namespace mandeye

#define LOG_INFO(msg) mandeye::Logger::Instance().Log(mandeye::LogLevel::Info, msg)
#define LOG_WARN(msg) mandeye::Logger::Instance().Log(mandeye::LogLevel::Warn, msg)
#define LOG_ERROR(msg) mandeye::Logger::Instance().Log(mandeye::LogLevel::Error, msg)
