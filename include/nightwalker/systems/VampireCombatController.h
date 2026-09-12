#pragma once

#include <cstdint>
#include <string_view>

#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/core/SafetyWatchdog.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameCombatApi.h"
#include "nightwalker/game/GameFeedingApi.h"
#include "nightwalker/game/GameMovementApi.h"
#include "nightwalker/game/GamePhysicalApi.h"
#include "nightwalker/systems/DebugVampireSpawner.h"
#include "nightwalker/systems/FeedingController.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::systems {

enum class CombatMove {
    ShadowstepStrike,
    HeavyStrike,
    GrabControl,
    GrabThrow,
    CombatFeed,
};

enum class CombatRole { PlayerDebug, Boss };

enum class CombatState {
    Idle,
    Telegraph,
    Align,
    Hold,
    Strike,
    Release,
    Feed,
    Recover,
    Abort,
};

class VampireCombatController final : public core::ILifecycleSystem {
public:
    VampireCombatController(
        game::IGameApi& api,
        game::IGameCombatApi& combatApi,
        game::IGameFeedingApi& feedingApi,
        game::IGameMovementApi& movementApi,
        game::IGamePhysicalApi& physicalApi,
        DebugVampireSpawner& spawner,
        FeedingController& feedingController,
        util::Logger& logger,
        core::Config& config) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "VampireCombatController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    bool RequestPlayerDebug(CombatMove move, std::uint64_t nowMs) noexcept;
    bool RequestBoss(CombatMove move, game::PedHandle actor, game::PedHandle target,
                     std::uint64_t nowMs, bool preTelegraphed = false) noexcept;
    bool RequestGrabFollowup(CombatMove move) noexcept;
    void CancelForActor(game::PedHandle actor) noexcept;

    [[nodiscard]] bool IsActive() const noexcept { return state_ != CombatState::Idle; }
    [[nodiscard]] bool IsActiveFor(game::PedHandle actor) const noexcept {
        return state_ != CombatState::Idle && actor_ == actor;
    }
    [[nodiscard]] CombatState State() const noexcept { return state_; }
    [[nodiscard]] CombatMove Move() const noexcept { return move_; }

private:
    bool BeginRequest(CombatMove move, CombatRole role, game::PedHandle actor,
                      game::PedHandle target, std::uint64_t nowMs, bool preTelegraphed) noexcept;
    bool ValidateInitial(bool directFeed) const noexcept;
    bool ValidateLive() const noexcept;
    void BeginStrike(std::uint64_t nowMs) noexcept;
    void BeginAlignment(std::uint64_t nowMs) noexcept;
    void BeginHold(std::uint64_t nowMs) noexcept;
    void BeginRelease(std::uint64_t nowMs) noexcept;
    void BeginFeed(std::uint64_t nowMs, bool fromOwnedHold) noexcept;
    void ApplyStrikeBonus() noexcept;
    void ApplyFeedResult() noexcept;
    void Enter(CombatState next, std::uint64_t nowMs) noexcept;
    void Abort(std::string_view reason) noexcept;
    void CleanupOwnedState() noexcept;
    void Reset() noexcept;
    [[nodiscard]] int WindupMs() const noexcept;
    [[nodiscard]] bool TimedOut(std::uint64_t nowMs) const noexcept;
    [[nodiscard]] std::uint64_t Elapsed(std::uint64_t nowMs) const noexcept;
    [[nodiscard]] static const char* StateName(CombatState state) noexcept;
    [[nodiscard]] static const char* MoveName(CombatMove move) noexcept;

    game::IGameApi& api_;
    game::IGameCombatApi& combatApi_;
    game::IGameFeedingApi& feedingApi_;
    game::IGameMovementApi& movementApi_;
    game::IGamePhysicalApi& physicalApi_;
    DebugVampireSpawner& spawner_;
    FeedingController& feedingController_;
    util::Logger& logger_;
    core::Config& config_;
    core::SafetyWatchdog watchdog_{};

    CombatState state_{CombatState::Idle};
    CombatMove move_{CombatMove::HeavyStrike};
    CombatRole role_{CombatRole::PlayerDebug};
    game::PedHandle actor_{0};
    game::PedHandle target_{0};
    std::uint64_t stateStartedMs_{0};
    bool preTelegraphed_{false};
    bool actorTaskOwned_{false};
    bool targetTaskOwned_{false};
    bool strikeBonusApplied_{false};
    bool feedResultApplied_{false};
};

} // namespace nightwalker::systems
