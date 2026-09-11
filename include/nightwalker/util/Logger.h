#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

namespace nightwalker::util {

enum class LogLevel { Info, Warning, Error };

class Logger final {
public:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool Initialize(const std::filesystem::path& logPath);
    void Shutdown();
    void Write(LogLevel level, std::string_view message);

private:
    std::mutex mutex_{};
    std::ofstream stream_{};
};

}
