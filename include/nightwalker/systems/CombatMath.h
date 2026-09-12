#pragma once

#include "nightwalker/game/GameApi.h"

namespace nightwalker::systems::combat_math {

struct ThrowPlan final {
    bool valid{false};
    game::Vec3 impulse{};
    game::Vec3 projectedEnd{};
};

[[nodiscard]] ThrowPlan BuildThrowPlan(
    const game::Vec3& actor,
    const game::Vec3& target,
    double horizontalForce,
    double upForce,
    double projectionMeters) noexcept;

[[nodiscard]] bool WithinRange(
    const game::Vec3& a,
    const game::Vec3& b,
    double maxDistance) noexcept;

} // namespace nightwalker::systems::combat_math
