#pragma once

#include <cstdint>

#include "nightwalker/game/GameApi.h"

namespace nightwalker::game {

class IGameEncounterApi {
public:
    virtual ~IGameEncounterApi() = default;
    [[nodiscard]] virtual int ClockHour() const noexcept = 0;
    [[nodiscard]] virtual std::int64_t GameSecondsSinceBaseYear() const noexcept = 0;
    [[nodiscard]] virtual bool IsSphereVisible(const Vec3& center, float radius) const noexcept = 0;
};

class GameEncounterApi final : public IGameEncounterApi {
public:
    [[nodiscard]] int ClockHour() const noexcept override;
    [[nodiscard]] std::int64_t GameSecondsSinceBaseYear() const noexcept override;
    [[nodiscard]] bool IsSphereVisible(const Vec3& center, float radius) const noexcept override;
};

} // namespace nightwalker::game
