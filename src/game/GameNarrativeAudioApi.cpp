#include "nightwalker/game/GameNarrativeAudioApi.h"

#include <Windows.h>
#include <mmsystem.h>

#include <fstream>
#include <sstream>
#include <system_error>

#include "nightwalker/util/Logger.h"

namespace nightwalker::game {

bool GameNarrativeAudioApi::Initialize(const std::filesystem::path& pluginDirectory, util::Logger& logger) noexcept {
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

void GameNarrativeAudioApi::Reload() noexcept {
    Stop();
    manifest_.Clear();
    warned_.clear();
    if (manifestPath_.empty()) return;

    try {
        std::ifstream input(manifestPath_);
        if (!input) {
            if (logger_) {
                logger_->Write(util::LogLevel::Info,
                    "Nightwalker.audio not found; using audio/<audio-id>.wav filename fallback.");
            }
            return;
        }
        std::ostringstream buffer;
        buffer << input.rdbuf();
        const bool parsed = manifest_.Parse(buffer.str(), [this](std::string_view message) {
            if (logger_) logger_->Write(util::LogLevel::Warning, message);
        });
        if (!parsed) {
            manifest_.Clear();
            if (logger_) {
                logger_->Write(util::LogLevel::Warning,
                    "Nightwalker.audio rejected; using audio/<audio-id>.wav filename fallback.");
            }
            return;
        }
        if (logger_) {
            logger_->Write(util::LogLevel::Info,
                "Nightwalker.audio loaded: " + std::to_string(manifest_.Count()) + " explicit mapping(s).");
        }
    } catch (...) {
        manifest_.Clear();
        if (logger_) {
            logger_->Write(util::LogLevel::Error,
                "Nightwalker.audio reload failed; using filename fallback and subtitles.");
        }
    }
}

void GameNarrativeAudioApi::Shutdown() noexcept {
    Stop();
    manifest_.Clear();
    warned_.clear();
    pluginDirectory_.clear();
    manifestPath_.clear();
    logger_ = nullptr;
}

bool GameNarrativeAudioApi::TryPlay(std::string_view assetId) noexcept {
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
            resolved->c_str(),
            nullptr,
            SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
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

void GameNarrativeAudioApi::Stop() noexcept {
    ::PlaySoundW(nullptr, nullptr, 0);
}

void GameNarrativeAudioApi::WarnOnce(std::string_view key, std::string_view message) noexcept {
    try {
        const std::string ownedKey(key);
        if (!warned_.insert(ownedKey).second) return;
        if (logger_) logger_->Write(util::LogLevel::Warning, message);
    } catch (...) {
        if (logger_) logger_->Write(util::LogLevel::Warning, message);
    }
}

} // namespace nightwalker::game
