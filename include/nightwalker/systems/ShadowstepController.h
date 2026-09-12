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
#include "nightwalker/systems/ShadowstepPresentationSettings.h"
#include "nightwalker/systems/ShadowstepResolver.h"
#include "nightwalker/systems/TargetedShadowstepPlanner.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::systems {

enum class ShadowstepState {
    Idle,
    ResolveIntent,
    ValidateDestination,
    Depart,
    Relocate,
    HiddenTransit,
    Arrive,
    ArrivalCarry,
    MeleeWindow,
    Recovery,
    Cooldown,
    Error,
};

class ShadowstepController final : public core::ILifecycleSystem {
public:
    ShadowstepController(
        game::IGameApi& api,
        game::IGameCombatApi& combatApi,
        util::Logger& logger,
        const core::Config& config) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "ShadowstepController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    void RequestForward(std::uint64_t nowMs) noexcept;
    void ReloadPresentationSettings(const std::filesystem::path& iniPath) noexcept;
    [[nodiscard]] ShadowstepState State() const noexcept { return state_; }
    [[nodiscard]] std::uint32_t StressSuccessCount() const noexcept { return stressSuccessCount_; }

private:
    void Transition(ShadowstepState next, std::uint64_t nowMs) noexcept;
    void Fail(const char* reason, std::uint64_t nowMs) noexcept;
    void ClearTransient() noexcept;
    void RestorePresentation() noexcept;
    void SampleMelee(std::uint64_t nowMs) noexcept;
    bool BeginHiddenTransit(std::uint64_t nowMs) noexcept;
    bool UpdateArrivalCarry(std::uint64_t nowMs) noexcept;
    bool StateTimedOut(std::uint64_t nowMs) const noexcept;
    [[nodiscard]] bool ValidationTimedOut(std::uint64_t nowMs) const noexcept;
    [[nodiscard]] static const char* StateName(ShadowstepState state) noexcept;

    game::IGameApi& api_;
    game::IGameCombatApi& combatApi_;
    util::Logger& logger_;
    const core::Config& config_;
    ShadowstepResolver resolver_;
    TargetedShadowstepPlanner targetedPlanner_;
    core::ShadowstepSettings carryResolverSettings_{};
    ShadowstepResolver carryResolver_;
    game::GamePresentationApi presentationApi_{};
    core::SafetyWatchdog presentationWatchdog_{};
    ShadowstepPresentationSettings presentationSettings_{};

    ShadowstepState state_{ShadowstepState::Idle};
    game::PedHandle player_{0};
    game::PedHandle targetPed_{0};
    game::Vec3 startPosition_{};
    game::Vec3 forwardDirection_{};
    game::Vec3 carryStart_{};
    ShadowstepResolveResult resolution_{};
    ShadowstepResolveResult carryResolution_{};
    TargetedShadowstepPlan targetedPlan_{};
    std::uint64_t stateStartedMs_{0};
    std::uint64_t cooldownUntilMs_{0};
    std::uint64_t hiddenUntilMs_{0};
    std::uint64_t meleeBufferUntilMs_{0};
    std::uint32_t stressSuccessCount_{0};
    bool targetedMode_{false};
    bool meleeBuffered_{false};
    bool fxUnavailableLogged_{false};
};

} // namespace nightwalker::systems
