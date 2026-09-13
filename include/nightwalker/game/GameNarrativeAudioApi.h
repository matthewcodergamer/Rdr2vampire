#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>

#include "nightwalker/narrative/NarrativeAudioManifest.h"

#ifdef _WIN32
#include <Windows.h>
#include <mmsystem.h>
#include "nightwalker/util/Logger.h"
#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")
#endif
#endif

namespace nightwalker::util { class Logger; }

namespace nightwalker::game {

class IGameNarrativeAudioApi {
public:
    virtual ~IGameNarrativeAudioApi() = default;
    virtual bool TryPlay(std::string_view assetId) noexcept = 0;
    virtual void Stop() noexcept = 0;
};

class GameNarrativeAudioApi final : public IGameNarrativeAudioApi {
public:
    bool Initialize(const std::filesystem::path& pluginDirectory, util::Logger& logger) noexcept;
    void Reload() noexcept;
    void Shutdown() noexcept;

    bool TryPlay(std::string_view assetId) noexcept override;
    void Stop() noexcept override;

    [[nodiscard]] bool Ready() const noexcept { return !pluginDirectory_.empty(); }

private:
    void WarnOnce(std::string_view key, std::string_view message) noexcept;

    std::filesystem::path pluginDirectory_{};
    std::filesystem::path manifestPath_{};
    narrative::NarrativeAudioManifest manifest_{};
    util::Logger* logger_{nullptr};
    std::unordered_set<std::string> warned_{};
};

class SubtitleOnlyNarrativeAudioApi final : public IGameNarrativeAudioApi {
public:
    bool TryPlay(std::string_view) noexcept override { return false; }
    void Stop() noexcept override {}
};

#ifdef _WIN32
inline bool GameNarrativeAudioApi::Initialize(
    const std::filesystem::path& pluginDirectory,
    util::Logger& logger) noexcept {
    try {
        Stop();
        pluginDirectory_ = pluginDirectory;
        manifestPath_ = pluginDirectory_ / L"Nightwalker.audio";
        logger_ = &logger;
        warned_.clear();
        Reload();
        logger_->Write(util::LogLevel::Info,
            "Narrative WAV backend initialized; missing voice assets fall back to subtitles.");
        return true;
    } catch (...) {
        pluginDirectory_.clear();
        manifestPath_.clear();
        manifest_.Clear();
        logger_ = &logger;
        logger_->Write(util::LogLevel::Error,
            "Narrative WAV backend initialization failed; subtitle fallback remains active.");
        return false;
    }
}

inline void GameNarrativeAudioApi::Reload() noexcept {
    Stop();
    manifest_.Clear();
    warned_.clear();
    if (manifestPath_.empty()) return;
    try {
        std::ifstream input(manifestPath_);
        if (!input) {
            if (logger_) logger_->Write(util::LogLevel::Info,
                "Nightwalker.audio not found; using audio/<audio-id>.wav filename fallback.");
            return;
        }
        std::ostringstream buffer;
        buffer << input.rdbuf();
        if (!manifest_.Parse(buffer.str(), [this](std::string_view message) {
                if (logger_) logger_->Write(util::LogLevel::Warning, message);
            })) {
            manifest_.Clear();
            if (logger_) logger_->Write(util::LogLevel::Warning,
                "Nightwalker.audio rejected; using audio/<audio-id>.wav filename fallback.");
            return;
        }
        if (logger_) logger_->Write(util::LogLevel::Info,
            "Nightwalker.audio loaded: " + std::to_string(manifest_.Count()) + " explicit mapping(s).");
    } catch (...) {
        manifest_.Clear();
        if (logger_) logger_->Write(util::LogLevel::Error,
            "Nightwalker.audio reload failed; using filename fallback and subtitles.");
    }
}

inline void GameNarrativeAudioApi::Shutdown() noexcept {
    Stop();
    manifest_.Clear();
    warned_.clear();
    pluginDirectory_.clear();
    manifestPath_.clear();
    logger_ = nullptr;
}

inline bool GameNarrativeAudioApi::TryPlay(std::string_view assetId) noexcept {
    if (pluginDirectory_.empty()) return false;
    try {
        const auto resolved = manifest_.Resolve(assetId, pluginDirectory_);
        if (!resolved) {
            WarnOnce(assetId, "Rejected unsafe narrative audio id; subtitle fallback active.");
            return false;
        }
        std::error_code error;
        if (!std::filesystem::is_regular_file(*resolved, error) || error) {
            WarnOnce(assetId,
                std::string("Narrative audio missing for '") + std::string(assetId) +
                "'; expected " + resolved->string());
            return false;
        }
        Stop();
        const BOOL played = ::PlaySoundW(
            resolved->c_str(), nullptr, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
        if (!played) {
            WarnOnce(assetId,
                std::string("Windows audio playback failed for '") + std::string(assetId) + "'.");
            return false;
        }
        return true;
    } catch (...) {
        WarnOnce(assetId, "Narrative audio playback threw unexpectedly; subtitle fallback active.");
        return false;
    }
}

inline void GameNarrativeAudioApi::Stop() noexcept {
    ::PlaySoundW(nullptr, nullptr, 0);
}

inline void GameNarrativeAudioApi::WarnOnce(std::string_view key, std::string_view message) noexcept {
    try {
        if (!warned_.insert(std::string(key)).second) return;
        if (logger_) logger_->Write(util::LogLevel::Warning, message);
    } catch (...) {
        if (logger_) logger_->Write(util::LogLevel::Warning, message);
    }
}
#endif

} // namespace nightwalker::game
