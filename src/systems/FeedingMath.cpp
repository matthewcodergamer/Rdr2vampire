#include "nightwalker/systems/FeedingMath.h"
#include <algorithm>
#include <cmath>

namespace nightwalker::systems::feeding_math {

double DistanceSquared(const game::Vec3& a, const game::Vec3& b) noexcept {
    const double dx = static_cast<double>(a.x) - b.x;
    const double dy = static_cast<double>(a.y) - b.y;
    const double dz = static_cast<double>(a.z) - b.z;
    return dx * dx + dy * dy + dz * dz;
}

bool WithinRange(const game::Vec3& a, const game::Vec3& b, double maxDistance) noexcept {
    const double limit = std::max(0.0, maxDistance);
    return DistanceSquared(a, b) <= limit * limit;
}

bool VerticalAligned(const game::Vec3& a, const game::Vec3& b, double maxDelta) noexcept {
    return std::abs(static_cast<double>(a.z) - b.z) <= std::max(0.0, maxDelta);
}

} // namespace nightwalker::systems::feeding_math
