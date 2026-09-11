#pragma once

#include <cstdint>
#include <string_view>

#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/systems/ShadowstepResolver.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::systems {

enum class ShadowstepState {
    Idle,
    ResolveIntent,
    ValidateDestination,
    Depart,
    Relocate,
    Arrive,
    Recovery,
    Cooldown,
    Error,
};

class ShadowstepController final : public core::ILifecycleSystem {
public:
    ShadowstepController(game::IGameApi& api, util::Logger& logger, const core::Config& config) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "ShadowstepController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    void RequestForward(std::uint64_t nowMs) noexcept;
    [[nodiscard]] ShadowstepState State() const noexcept { return state_; }
    [[nodiscard]] std::uint32_t StressSuccessCount() const noexcept { return stressSuccessCount_; }

private:
    void Transition(ShadowstepState next, std::uint64_t nowMs) noexcept;
    void Fail(const char* reason, std::uint64_t nowMs) noexcept;
    void ClearTransient() noexcept;
    [[nodiscard]] bool ValidationTimedOut(std::uint64_t nowMs) const noexcept;
    [[nodiscard]] static const char* StateName(ShadowstepState state) noexcept;

    game::IGameApi& api_;
    util::Logger& logger_;
    const core::Config& config_;
    ShadowstepResolver resolver_;

    ShadowstepState state_{ShadowstepState::Idle};
    game::PedHandle player_{0};
    game::Vec3 startPosition_{};
    game::Vec3 forwardDirection_{};
    ShadowstepResolveResult resolution_{};
    std::uint64_t stateStartedMs_{0};
    std::uint64_t cooldownUntilMs_{0};
    std::uint32_t stressSuccessCount_{0};
};

} // namespace nightwalker::systems
