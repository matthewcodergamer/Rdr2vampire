#pragma once

#include <cstdint>

#include "nightwalker/game/GameApi.h"

namespace nightwalker::systems::movement_math {

[[nodiscard]] float SmoothStep01(float value) noexcept;
[[nodiscard]] float RampMultiplier(
    std::uint64_t elapsedMs,
    std::uint32_t rampDurationMs,
    float targetMultiplier) noexcept;
[[nodiscard]] float HorizontalSpeed(const game::Vec3& velocity) noexcept;

} // namespace nightwalker::systems::movement_math
