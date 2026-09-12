#pragma once

#include <cstdint>
#include <string_view>

#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/core/SafetyWatchdog.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameCombatApi.h"
#include "nightwalker/game/GameMovementApi.h"
#include "nightwalker/game/GamePresentationApi.h"
#include "nightwalker/systems/DebugVampireSpawner.h"
#include "nightwalker/systems/VampireAIController.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::systems {

enum class MovementState {
    Idle,
    RampUp,
    Boost,
    Recovery,
    Restricted,
};

class MovementController final : public core::ILifecycleSystem {
public:
    MovementController(
        game::IGameApi& api,
        game::IGameCombatApi& combatApi,
        game::IGameMovementApi& movementApi,
        game::IGamePresentationApi& presentationApi,
        DebugVampireSpawner& spawner,
        VampireAIController& vampireAi,
        util::Logger& logger,
        const core::Config& config) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "MovementController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    [[nodiscard]] MovementState State() const noexcept { return state_; }
    [[nodiscard]] float LastAppliedMultiplier() const noexcept { return lastAppliedMultiplier_; }

private:
    [[nodiscard]] bool ValidOwnedVampire(game::PedHandle ped) const noexcept;
    [[nodiscard]] bool IsMovementRestricted(game::PedHandle ped, std::uint64_t nowMs) noexcept;
    [[nodiscard]] bool WantsBoost(game::PedHandle vampire, game::PedHandle player) const noexcept;
    bool ApplyMultiplier(game::PedHandle ped, float multiplier) noexcept;
    void RestoreMultiplier(std::string_view reason) noexcept;
    void EmitTrail(game::PedHandle ped, std::uint64_t nowMs) noexcept;
    void Transition(MovementState next, std::uint64_t nowMs) noexcept;
    void ResetTransient() noexcept;
    [[nodiscard]] static const char* StateName(MovementState state) noexcept;

    game::IGameApi& api_;
    game::IGameCombatApi& combatApi_;
    game::IGameMovementApi& movementApi_;
    game::IGamePresentationApi& presentationApi_;
    DebugVampireSpawner& spawner_;
    VampireAIController& vampireAi_;
    util::Logger& logger_;
    const core::Config& config_;
    core::SafetyWatchdog movementWatchdog_{};

    MovementState state_{MovementState::Idle};
    game::PedHandle ownedPed_{0};
    std::uint64_t stateStartedMs_{0};
    std::uint64_t recoveryUntilMs_{0};
    std::uint64_t dismountRecoveryUntilMs_{0};
    std::uint64_t nextTrailMs_{0};
    float lastAppliedMultiplier_{1.0F};
    bool wasMounted_{false};
    bool trailUnavailableLogged_{false};
};

} // namespace nightwalker::systems
