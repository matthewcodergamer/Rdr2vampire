#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/game/GameBossBarApi.h"
#include "nightwalker/game/GameNarrativeAudioApi.h"
#include "nightwalker/narrative/NarrativePlayback.h"
#include "nightwalker/narrative/NarrativeScript.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::narrative {

class NarrativeController final : public core::ILifecycleSystem {
public:
    NarrativeController(game::IGameBossBarApi& textApi,
                        game::IGameNarrativeAudioApi& audioApi,
                        util::Logger& logger,
                        const core::Config& config) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "NarrativeController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    void LoadScript(const std::filesystem::path& path);
    bool StartSequence(std::string_view sequenceId, std::uint64_t nowMs) noexcept;
    bool StartSequenceFamily(std::string_view familyId, std::uint64_t nowMs) noexcept;
    void RequestSkip(std::uint64_t nowMs) noexcept;

    [[nodiscard]] bool Active() const noexcept { return playback_.Active(); }
    [[nodiscard]] bool IsPlaying(std::string_view sequenceId) const noexcept {
        return playback_.IsPlaying(sequenceId);
    }
    [[nodiscard]] bool IsPlayingFamily(std::string_view familyId) const noexcept {
        return playback_.Active() && SequenceBelongsToFamily(playback_.SequenceId(), familyId);
    }
    [[nodiscard]] std::uint64_t RemainingMs(std::uint64_t nowMs) const noexcept {
        return playback_.RemainingMs(nowMs);
    }

private:
    bool StartResolvedSequence(const NarrativeSequence& sequence, std::uint64_t nowMs) noexcept;
    bool SkipKeyDown() const noexcept;
    void PresentAudioForCurrentLine() noexcept;
    void DrawCurrentSubtitle() noexcept;

    game::IGameBossBarApi& textApi_;
    game::IGameNarrativeAudioApi& audioApi_;
    util::Logger& logger_;
    const core::Config& config_;
    NarrativeCatalog catalog_{};
    NarrativeVariantSelector variants_{};
    NarrativePlayback playback_{};
    std::filesystem::path scriptPath_{};
    std::string presentedLineId_{};
    std::uint64_t variationNonce_{0};
    bool skipWasDown_{false};
};

} // namespace nightwalker::narrative
