#pragma once
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>
namespace nightwalker::util {
enum class LogLevel { Debug=0, Info=1, Warning=2, Error=3 };
class Logger final {
public:
 Logger()=default; Logger(const Logger&)=delete; Logger& operator=(const Logger&)=delete;
 bool Initialize(const std::filesystem::path& logPath);
 void Shutdown() noexcept;
 void SetMinimumLevel(LogLevel level) noexcept { minimumLevel_=level; }
 void Write(LogLevel level,std::string_view message) noexcept;
private:
 std::mutex mutex_{}; std::ofstream stream_{}; LogLevel minimumLevel_{LogLevel::Info};
};
}
