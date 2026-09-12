#include "nightwalker/game/GameEncounterApi.h"

#include <algorithm>
#include <natives.h>

namespace nightwalker::game {

int GameEncounterApi::ClockHour() const noexcept {
    return std::clamp(CLOCK::GET_CLOCK_HOURS(), 0, 23);
}

std::int64_t GameEncounterApi::GameSecondsSinceBaseYear() const noexcept {
    const int seconds = CLOCK::_GET_SECONDS_SINCE_BASE_YEAR();
    return seconds < 0 ? 0 : static_cast<std::int64_t>(seconds);
}

bool GameEncounterApi::IsSphereVisible(const Vec3& center, float radius) const noexcept {
    return CAM::IS_SPHERE_VISIBLE(center.x, center.y, center.z, std::max(radius, 0.1F)) == TRUE;
}

} // namespace nightwalker::game
