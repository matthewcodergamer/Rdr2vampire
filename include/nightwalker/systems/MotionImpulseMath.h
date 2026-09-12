#pragma once

#include "nightwalker/game/GameApi.h"

namespace nightwalker::systems::motion_impulse_math {

struct Plan final {
    bool valid{false};
    game::Vec3 impulse{};
    game::Vec3 projectedEnd{};
};

[[nodiscard]] Plan BuildPlan(
    const game::Vec3& actor,
    const game::Vec3& target,
    double horizontalAmount,
    double upwardAmount,
    double projectionMeters) noexcept;

[[nodiscard]] bool WithinRange(
    const game::Vec3& a,
    const game::Vec3& b,
    double maxDistance) noexcept;

} // namespace nightwalker::systems::motion_impulse_math
