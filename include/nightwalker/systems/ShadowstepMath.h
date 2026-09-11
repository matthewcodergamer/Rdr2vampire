#pragma once

#include "nightwalker/game/GameApi.h"

namespace nightwalker::systems::shadowstep_math {

[[nodiscard]] float Distance2D(const game::Vec3& a, const game::Vec3& b) noexcept;
[[nodiscard]] float Distance3D(const game::Vec3& a, const game::Vec3& b) noexcept;
[[nodiscard]] bool NormalizeHorizontal(const game::Vec3& input, game::Vec3& output) noexcept;
[[nodiscard]] game::Vec3 AddScaled(const game::Vec3& origin, const game::Vec3& direction, float distance) noexcept;
[[nodiscard]] game::Vec3 PullBackFromHit(const game::Vec3& hit, const game::Vec3& direction, float clearance) noexcept;
[[nodiscard]] bool VerticalDeltaWithin(const game::Vec3& start, const game::Vec3& end, float maximumDelta) noexcept;

} // namespace nightwalker::systems::shadowstep_math
