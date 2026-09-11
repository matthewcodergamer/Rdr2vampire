#include "nightwalker/util/Logger.h"
#include <Windows.h>
#include <array>
#include <cstdio>
#include <string>

namespace nightwalker::util {
namespace {
const char* ToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Timestamp() {
    SYSTEMTIME value{};
    ::GetLocalTime(&value);
    std::array<char, 32> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%04u-%02u-%02u %02u:%02u:%02u.%03u",
        static_cast<unsigned>(value.wYear), static_cast<unsigned>(value.wMonth),
        static_cast<unsigned>(value.wDay), static_cast<unsigned>(value.wHour),
        static_cast<unsigned>(value.wMinute), static_cast<unsigned>(value.wSecond),
        static_cast<unsigned>(value.wMilliseconds));
    return std::string(buffer.data());
}
}

bool Logger::Initialize(const std::filesystem::path& logPath) {
    std::scoped_lock lock(mutex_);
    if (stream_.is_open()) stream_.close();
    stream_.clear();
    stream_.open(logPath, std::ios::out | std::ios::app);
    return stream_.is_open();
}

void Logger::Shutdown() {
    std::scoped_lock lock(mutex_);
    if (stream_.is_open()) {
        stream_.flush();
        stream_.close();
    }
}

void Logger::Write(LogLevel level, std::string_view message) {
    const std::string line = "[" + Timestamp() + "] [" + ToString(level) + "] " + std::string(message);
    ::OutputDebugStringA((line + "\n").c_str());
    std::scoped_lock lock(mutex_);
    if (stream_.is_open()) {
        stream_ << line << '\n';
        stream_.flush();
    }
}
}
