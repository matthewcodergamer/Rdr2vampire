#pragma once

#include "nightwalker/core/Config.h"
#include "nightwalker/game/GameApi.h"

namespace nightwalker::systems {

enum class ShadowstepRejectReason {
    None,
    InvalidPlayer,
    InvalidDirection,
    TraceInconclusive,
    ObstructedTooClose,
    UnsafeNavmesh,
    SafePointTooFar,
    GroundUnavailable,
    VerticalDeltaTooLarge,
    DeepWater,
    ClearanceBlocked,
    HeadroomBlocked,
};

struct ShadowstepResolveResult final {
    bool valid{false};
    bool shortened{false};
    game::Vec3 requested{};
    game::Vec3 finalPosition{};
    float requestedDistance{0.0F};
    float finalDistance{0.0F};
    ShadowstepRejectReason reason{ShadowstepRejectReason::None};
};

class ShadowstepResolver final {
public:
    ShadowstepResolver(game::IGameApi& api, const core::ShadowstepSettings& settings) noexcept;

    [[nodiscard]] ShadowstepResolveResult Resolve(
        game::PedHandle player,
        const game::Vec3& start,
        const game::Vec3& desiredDirection) const noexcept;

    [[nodiscard]] static const char* ReasonText(ShadowstepRejectReason reason) noexcept;

private:
    [[nodiscard]] bool HasClearance(game::PedHandle player, const game::Vec3& position,
                                    ShadowstepRejectReason& rejection) const noexcept;

    game::IGameApi& api_;
    const core::ShadowstepSettings& settings_;
};

} // namespace nightwalker::systems
