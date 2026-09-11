#include "nightwalker/systems/ShadowstepMath.h"

#include <cmath>

namespace nightwalker::systems::shadowstep_math {

float Distance2D(const game::Vec3& a, const game::Vec3& b) noexcept {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

float Distance3D(const game::Vec3& a, const game::Vec3& b) noexcept {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool NormalizeHorizontal(const game::Vec3& input, game::Vec3& output) noexcept {
    const float length = std::sqrt(input.x * input.x + input.y * input.y);
    if (length < 0.0001F) {
        output = {};
        return false;
    }
    output = {input.x / length, input.y / length, 0.0F};
    return true;
}

game::Vec3 AddScaled(const game::Vec3& origin, const game::Vec3& direction, float distance) noexcept {
    return {
        origin.x + direction.x * distance,
        origin.y + direction.y * distance,
        origin.z + direction.z * distance,
    };
}

game::Vec3 PullBackFromHit(const game::Vec3& hit, const game::Vec3& direction, float clearance) noexcept {
    return {
        hit.x - direction.x * clearance,
        hit.y - direction.y * clearance,
        hit.z - direction.z * clearance,
    };
}

bool VerticalDeltaWithin(const game::Vec3& start, const game::Vec3& end, float maximumDelta) noexcept {
    return std::fabs(end.z - start.z) <= maximumDelta;
}

} // namespace nightwalker::systems::shadowstep_math
