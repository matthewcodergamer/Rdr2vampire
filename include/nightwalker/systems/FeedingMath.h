#pragma once
#include "nightwalker/game/GameApi.h"

namespace nightwalker::systems::feeding_math {

double DistanceSquared(const game::Vec3& a, const game::Vec3& b) noexcept;
bool WithinRange(const game::Vec3& a, const game::Vec3& b, double maxDistance) noexcept;
bool VerticalAligned(const game::Vec3& a, const game::Vec3& b, double maxDelta) noexcept;

} // namespace nightwalker::systems::feeding_math
