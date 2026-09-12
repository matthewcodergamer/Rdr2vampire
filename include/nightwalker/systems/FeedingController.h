#pragma once

#include <cstdint>
#include <string_view>
#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameCombatApi.h"
#include "nightwalker/game/GameFeedingApi.h"
#include "nightwalker/systems/HiddenResource.h"
#include "nightwalker/systems/VampireFeedPresentation.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::systems {

enum class FeedMode { Sip, Drain };
enum class FeedingState { Idle, Candidate, Align, Grab, FeedLoop, ReleaseDrain, Cleanup };

class FeedingController final : public core::ILifecycleSystem {
public:
    FeedingController(game::IGameApi& gameApi,
                      game::IGameCombatApi& combatApi,
                      game::IGameFeedingApi& feedingApi,
                      util::Logger& logger,
                      core::Config& config) noexcept;

    std::string_view Name() const noexcept override { return "FeedingController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    bool Request(FeedMode mode, std::uint64_t nowMs) noexcept;
    void GainHiddenResource(double amount) noexcept;
    [[nodiscard]] bool IsActive() const noexcept { return state_ != FeedingState::Idle; }
    [[nodiscard]] FeedingState State() const noexcept { return state_; }
    [[nodiscard]] double HiddenResourceValue() const noexcept { return resource_.Value(); }

private:
    bool AcquireCandidate(std::uint64_t nowMs) noexcept;
    bool ValidateParticipants(const char*& reason) const noexcept;
    bool WithinDistance(double extra = 0.0) const noexcept;
    void Enter(FeedingState state, std::uint64_t nowMs) noexcept;
    void BeginAlignment() noexcept;
    void BeginGrab() noexcept;
    void ApplyCompletion() noexcept;
    void Abort(std::string_view reason) noexcept;
    void CleanupOwnedTasks() noexcept;
    void ResetInteraction() noexcept;
    bool StateTimedOut(std::uint64_t nowMs) const noexcept;
    std::uint64_t StateElapsed(std::uint64_t nowMs) const noexcept;
    static const char* StateName(FeedingState state) noexcept;
    static const char* ModeName(FeedMode mode) noexcept;

    game::IGameApi& gameApi_;
    game::IGameCombatApi& combatApi_;
    game::IGameFeedingApi& feedingApi_;
    util::Logger& logger_;
    core::Config& config_;
    VampireFeedPresentation feedPresentation_;
    HiddenResource resource_{};

    FeedingState state_{FeedingState::Idle};
    FeedMode mode_{FeedMode::Sip};
    game::PedHandle player_{0};
    game::PedHandle target_{0};
    std::uint64_t stateStartedMs_{0};
    bool playerTaskOwned_{false};
    bool targetTaskOwned_{false};
    bool completionApplied_{false};
};

} // namespace nightwalker::systems
