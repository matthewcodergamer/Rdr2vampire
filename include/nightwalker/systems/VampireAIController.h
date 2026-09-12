#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/core/SafetyWatchdog.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameCombatApi.h"
#include "nightwalker/game/GamePresentationApi.h"
#include "nightwalker/systems/DebugVampireSpawner.h"
#include "nightwalker/systems/ShadowstepPresentationSettings.h"
#include "nightwalker/systems/ShadowstepResolver.h"
#include "nightwalker/systems/TargetedShadowstepPlanner.h"
#include "nightwalker/systems/VampireCombatController.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::systems {

enum class VampireAiState {
    Observe,
    Approach,
    Decide,
    ShadowstepDepart,
    HiddenTransit,
    ShadowstepArrive,
    Telegraph,
    Attack,
    MeleeAbility,
    Recover,
    Cooldown,
    Evade,
    Reposition,
    FeedAttempt,
    Abort,
};

class VampireAIController final : public core::ILifecycleSystem {
public:
    VampireAIController(
        game::IGameApi& api,
        game::IGameCombatApi& combatApi,
        game::IGamePresentationApi& presentationApi,
        DebugVampireSpawner& spawner,
        VampireCombatController& combatController,
        util::Logger& logger,
        const core::Config& config) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "VampireAIController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    void ReloadPresentationSettings(const std::filesystem::path& iniPath) noexcept;
    [[nodiscard]] VampireAiState State() const noexcept { return state_; }
    [[nodiscard]] ShadowstepCandidateType LastCandidate() const noexcept { return lastCandidate_; }

private:
    void Transition(VampireAiState next, std::uint64_t nowMs) noexcept;
    void Abort(const char* reason, std::uint64_t nowMs) noexcept;
    void RestoreOwnedPresentation() noexcept;
    void ResetTransient() noexcept;
    void EnsureOrdinaryCombat() noexcept;
    void LogPlan(const TargetedShadowstepPlan& plan) const;
    [[nodiscard]] bool ValidCombatActors() const noexcept;
    [[nodiscard]] bool PlayerRetreating() const noexcept;
    [[nodiscard]] bool BeginShadowstep(std::uint64_t nowMs) noexcept;
    [[nodiscard]] bool UpdateArrivalCarry(std::uint64_t nowMs) noexcept;
    [[nodiscard]] bool StateTimedOut(std::uint64_t nowMs) const noexcept;
    [[nodiscard]] CombatMove NextCloseCombatMove() noexcept;
    [[nodiscard]] static const char* StateName(VampireAiState state) noexcept;

    game::IGameApi& api_;
    game::IGameCombatApi& combatApi_;
    game::IGamePresentationApi& presentationApi_;
    DebugVampireSpawner& spawner_;
    VampireCombatController& combatController_;
    util::Logger& logger_;
    const core::Config& config_;

    ShadowstepResolver resolver_;
    TargetedShadowstepPlanner planner_;
    ShadowstepPresentationSettings presentationSettings_{};
    core::SafetyWatchdog presentationWatchdog_{};

    VampireAiState state_{VampireAiState::Observe};
    game::PedHandle vampire_{0};
    game::PedHandle player_{0};
    TargetedShadowstepPlan plan_{};
    ShadowstepCandidateType lastCandidate_{ShadowstepCandidateType::Intercept};
    game::Vec3 departPosition_{};
    game::Vec3 carryStart_{};
    game::Vec3 carryDestination_{};
    std::uint64_t stateStartedMs_{0};
    std::uint64_t hiddenUntilMs_{0};
    std::uint64_t nextDecisionMs_{0};
    std::uint64_t shadowstepCooldownUntilMs_{0};
    std::uint64_t evadeCooldownUntilMs_{0};
    std::uint64_t bossSpecialCooldownUntilMs_{0};
    unsigned bossSpecialSequence_{0};
    bool evadeIntent_{false};
    bool evadeOpportunityToggle_{false};
    bool playerMeleeWasDown_{false};
};

} // namespace nightwalker::systems
