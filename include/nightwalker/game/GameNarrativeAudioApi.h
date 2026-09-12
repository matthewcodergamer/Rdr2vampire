#pragma once

#include <string_view>

namespace nightwalker::game {

class IGameNarrativeAudioApi {
public:
    virtual ~IGameNarrativeAudioApi() = default;
    virtual bool TryPlay(std::string_view assetId) noexcept = 0;
    virtual void Stop() noexcept = 0;
};

// Phase 12 ships subtitle-first. This implementation deliberately reports
// optional audio as unavailable until an approved, isolated audio backend is added.
class SubtitleOnlyNarrativeAudioApi final : public IGameNarrativeAudioApi {
public:
    bool TryPlay(std::string_view) noexcept override { return false; }
    void Stop() noexcept override {}
};

} // namespace nightwalker::game
