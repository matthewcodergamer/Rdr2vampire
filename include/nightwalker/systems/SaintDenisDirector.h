#pragma once

#include <cstdint>
#include <string_view>

#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameCombatApi.h"
#include "nightwalker/game/GameEncounterApi.h"
#include "nightwalker/game/GamePresentationApi.h"
#include "nightwalker/game/ModelStreamRequest.h"
#include "nightwalker/narrative/NarrativeController.h"
#include "nightwalker/systems/BossActorRegistry.h"
#include "nightwalker/ui/BossHudController.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::systems {

enum class SaintDenisState {
    Dormant, Eligible, Omen, SpawnPending, Stalking, Confrontation,
    Combat, Resolution, Cooldown, Abort, Cleanup,
};

class SaintDenisDirector final : public core::ILifecycleSystem {
public:
    SaintDenisDirector(game::IGameApi& api, game::IGameCombatApi& combatApi,
        game::IGameEncounterApi& encounterApi, game::IGamePresentationApi& presentationApi,
        BossActorRegistry& registry, narrative::NarrativeController& narrative,
        ui::BossHudController& bossHud, util::Logger& logger, const core::Config& config) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "EncounterDirector"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    [[nodiscard]] SaintDenisState State() const noexcept { return state_; }
    [[nodiscard]] game::PedHandle ActivePed() const noexcept { return actor_; }
    [[nodiscard]] bool ResolvedThisSession() const noexcept { return resolvedThisSession_; }
    [[nodiscard]] std::int64_t CooldownUntilGameSeconds() const noexcept { return cooldownUntilGameSeconds_; }

private:
    void UpdateSetup(const core::FrameContext& frame);
    void UpdateActive(const core::FrameContext& frame);
    void UpdateFinish(const core::FrameContext& frame);
    [[nodiscard]] bool EligibleNow() const noexcept;
    [[nodiscard]] bool PreCombatStillSafe() const noexcept;
    [[nodiscard]] bool ActorValid(bool requireAlive = true) const noexcept;
    [[nodiscard]] bool FindSpawnPoint(game::Vec3& position, float& heading, bool& visible) const noexcept;
    bool BeginModelRequest(std::uint64_t nowMs) noexcept;
    bool SpawnActor(std::uint64_t nowMs) noexcept;
    void BeginAbort(std::string_view reason, std::uint64_t nowMs) noexcept;
    [[nodiscard]] bool CleanupActor(bool resolved) noexcept;
    void Transition(SaintDenisState next, std::uint64_t nowMs) noexcept;
    [[nodiscard]] static const char* StateName(SaintDenisState state) noexcept;

    game::IGameApi& api_;
    game::IGameCombatApi& combatApi_;
    game::IGameEncounterApi& encounterApi_;
    game::IGamePresentationApi& presentationApi_;
    BossActorRegistry& registry_;
    narrative::NarrativeController& narrative_;
    ui::BossHudController& bossHud_;
    util::Logger& logger_;
    const core::Config& config_;
    game::ModelStreamRequest modelRequest_{};
    SaintDenisState state_{SaintDenisState::Dormant};
    game::PedHandle actor_{0};
    std::uint64_t stateStartedMs_{0};
    std::uint64_t nextEligibilityCheckMs_{0};
    std::uint64_t nextOmenMs_{0};
    std::uint64_t nextSpawnRetryMs_{0};
    std::uint64_t outsideSinceMs_{0};
    std::int64_t cooldownUntilGameSeconds_{0};
    bool cleanupResolved_{false};
    bool resolvedThisSession_{false};
};

} // namespace nightwalker::systems
