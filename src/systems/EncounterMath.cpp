#include "nightwalker/systems/EncounterMath.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace nightwalker::systems::encounter_math {

bool IsHourInWindow(int hour, int startHour, int endHour) noexcept {
    hour = std::clamp(hour, 0, 23);
    startHour = std::clamp(startHour, 0, 23);
    endHour = std::clamp(endHour, 0, 23);
    if (startHour == endHour) return true;
    if (startHour < endHour) return hour >= startHour && hour < endHour;
    return hour >= startHour || hour < endHour;
}

float Distance2D(const game::Vec3& a, const game::Vec3& b) noexcept {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool WithinRadius(const game::Vec3& a, const game::Vec3& b, float radius) noexcept {
    return radius > 0.0F && Distance2D(a, b) <= radius;
}

namespace {
std::int64_t SaturatingAdd(std::int64_t base, std::int64_t delta) noexcept {
    if (delta <= 0) return base;
    const auto max = std::numeric_limits<std::int64_t>::max();
    if (base > max - delta) return max;
    return base + delta;
}
}

std::int64_t AddCooldownHours(std::int64_t gameSeconds, int hours) noexcept {
    const auto safe = static_cast<std::int64_t>(std::max(hours, 0));
    return SaturatingAdd(std::max<std::int64_t>(gameSeconds, 0), safe * 3600);
}

std::int64_t AddCooldownMinutes(std::int64_t gameSeconds, int minutes) noexcept {
    const auto safe = static_cast<std::int64_t>(std::max(minutes, 0));
    return SaturatingAdd(std::max<std::int64_t>(gameSeconds, 0), safe * 60);
}

bool CooldownExpired(std::int64_t gameSeconds, std::int64_t cooldownUntil) noexcept {
    return cooldownUntil <= 0 || gameSeconds >= cooldownUntil;
}

} // namespace nightwalker::systems::encounter_math
