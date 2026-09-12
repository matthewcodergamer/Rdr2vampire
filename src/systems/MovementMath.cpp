#include "nightwalker/systems/MovementMath.h"

#include <algorithm>
#include <cmath>

namespace nightwalker::systems::movement_math {

float SmoothStep01(float value) noexcept {
    const float t = std::clamp(value, 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

float RampMultiplier(
    std::uint64_t elapsedMs,
    std::uint32_t rampDurationMs,
    float targetMultiplier) noexcept {
    const float target = std::max(1.0F, targetMultiplier);
    if (rampDurationMs == 0U) return target;
    const float t = static_cast<float>(elapsedMs) / static_cast<float>(rampDurationMs);
    return 1.0F + (target - 1.0F) * SmoothStep01(t);
}

float HorizontalSpeed(const game::Vec3& velocity) noexcept {
    return std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
}

} // namespace nightwalker::systems::movement_math
