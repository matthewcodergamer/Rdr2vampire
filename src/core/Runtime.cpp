#include "nightwalker/core/Runtime.h"
#include <Windows.h>
#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include "nightwalker/Version.h"

namespace nightwalker::core {
namespace {
bool ResolveModulePath(HMODULE moduleHandle, std::filesystem::path& result) {
    if (moduleHandle == nullptr) return false;
    std::array<wchar_t, 32768> buffer{};
    const DWORD capacity = static_cast<DWORD>(buffer.size());
    const DWORD length = ::GetModuleFileNameW(moduleHandle, buffer.data(), capacity);
    if (length == 0 || length >= capacity) return false;
    result = std::filesystem::path(std::wstring_view(buffer.data(), length));
    return true;
}
}

bool Runtime::Initialize(HMODULE moduleHandle) {
    if (initialized_) return true;

    std::filesystem::path modulePath;
    if (!ResolveModulePath(moduleHandle, modulePath) || !gameContext_.Initialize(std::move(modulePath))) {
        ::OutputDebugStringA("[Nightwalker] Failed to resolve plugin path.\n");
        return false;
    }

    const bool fileLogReady = logger_.Initialize(gameContext_.PluginDirectory() / L"Nightwalker.log");
    logger_.Write(util::LogLevel::Info,
        std::string("Starting ") + version::kProjectName + " " + version::kString + " (Story Mode only).");
    logger_.Write(util::LogLevel::Info, std::string("Plugin path: ") + gameContext_.ModulePath().string());
    if (!fileLogReady) {
        logger_.Write(util::LogLevel::Warning,
            "Nightwalker.log could not be opened; OutputDebugString diagnostics remain active.");
    }

    tickCount_ = 0;
    initialized_ = true;
    logger_.Write(util::LogLevel::Info, "Phase 0 runtime initialized; no gameplay systems are active.");
    return true;
}

void Runtime::Tick() {
    if (!initialized_) return;
    ++tickCount_;
}

void Runtime::Shutdown() {
    if (!initialized_) return;
    logger_.Write(util::LogLevel::Info,
        std::string("Runtime shutdown requested after ") + std::to_string(tickCount_) + " ticks.");
    initialized_ = false;
    tickCount_ = 0;
    gameContext_.Reset();
    logger_.Shutdown();
}
}
