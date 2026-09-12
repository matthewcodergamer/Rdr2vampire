#pragma once

#include <cstdint>

#include "nightwalker/game/GameApi.h"

namespace nightwalker::systems::encounter_math {

[[nodiscard]] bool IsHourInWindow(int hour, int startHour, int endHour) noexcept;
[[nodiscard]] float Distance2D(const game::Vec3& a, const game::Vec3& b) noexcept;
[[nodiscard]] bool WithinRadius(const game::Vec3& a, const game::Vec3& b, float radius) noexcept;
[[nodiscard]] std::int64_t AddCooldownHours(std::int64_t gameSeconds, int hours) noexcept;
[[nodiscard]] std::int64_t AddCooldownMinutes(std::int64_t gameSeconds, int minutes) noexcept;
[[nodiscard]] bool CooldownExpired(std::int64_t gameSeconds, std::int64_t cooldownUntil) noexcept;

} // namespace nightwalker::systems::encounter_math
