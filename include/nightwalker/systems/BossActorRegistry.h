#pragma once

#include "nightwalker/game/GameApi.h"

namespace nightwalker::systems {

inline constexpr game::ModelHash kSaintDenisVampireModel = 0xD95BCB7D;

enum class BossOwner {
    None,
    Debug,
    Encounter,
};

class BossActorRegistry final {
public:
    [[nodiscard]] bool Claim(game::PedHandle ped, BossOwner owner) noexcept;
    [[nodiscard]] bool Release(game::PedHandle ped, BossOwner owner) noexcept;
    [[nodiscard]] bool SetCombatEnabled(game::PedHandle ped, BossOwner owner, bool enabled) noexcept;
    void ForceClear() noexcept;

    [[nodiscard]] game::PedHandle Ped() const noexcept { return ped_; }
    [[nodiscard]] BossOwner Owner() const noexcept { return owner_; }
    [[nodiscard]] bool CombatEnabled() const noexcept { return combatEnabled_; }
    [[nodiscard]] bool IsOwnedBy(BossOwner owner) const noexcept { return ped_ != 0 && owner_ == owner; }
    [[nodiscard]] bool IsOwnedPed(game::PedHandle ped) const noexcept { return ped_ != 0 && ped_ == ped; }

private:
    game::PedHandle ped_{0};
    BossOwner owner_{BossOwner::None};
    bool combatEnabled_{false};
};

} // namespace nightwalker::systems
