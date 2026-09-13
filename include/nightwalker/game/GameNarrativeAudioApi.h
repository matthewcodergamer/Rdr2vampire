#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>

#include "nightwalker/narrative/NarrativeAudioManifest.h"

namespace nightwalker::util { class Logger; }

namespace nightwalker::game {

class IGameNarrativeAudioApi {
public:
    virtual ~IGameNarrativeAudioApi() = default;
    virtual bool TryPlay(std::string_view assetId) noexcept = 0;
    virtual void Stop() noexcept = 0;
};

// External original/licensed voice playback. Narrative lines remain subtitle-safe:
// if an asset is missing, invalid, or fails to play, TryPlay returns false and the
// NarrativeController continues normally with subtitles.
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

// Useful for deterministic tests and installations where custom audio is disabled.
class SubtitleOnlyNarrativeAudioApi final : public IGameNarrativeAudioApi {
public:
    bool TryPlay(std::string_view) noexcept override { return false; }
    void Stop() noexcept override {}
};

} // namespace nightwalker::game
