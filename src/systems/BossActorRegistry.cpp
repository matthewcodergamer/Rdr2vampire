#include "nightwalker/systems/BossActorRegistry.h"

namespace nightwalker::systems {

bool BossActorRegistry::Claim(game::PedHandle ped, BossOwner owner) noexcept {
    if (ped == 0 || owner == BossOwner::None) return false;
    if (ped_ != 0) return ped_ == ped && owner_ == owner;
    ped_ = ped;
    owner_ = owner;
    combatEnabled_ = false;
    return true;
}

bool BossActorRegistry::Release(game::PedHandle ped, BossOwner owner) noexcept {
    if (ped_ == 0) return true;
    if (ped_ != ped || owner_ != owner) return false;
    ForceClear();
    return true;
}

bool BossActorRegistry::SetCombatEnabled(game::PedHandle ped, BossOwner owner, bool enabled) noexcept {
    if (ped_ == 0 || ped_ != ped || owner_ != owner) return false;
    combatEnabled_ = enabled;
    return true;
}

void BossActorRegistry::ForceClear() noexcept {
    ped_ = 0;
    owner_ = BossOwner::None;
    combatEnabled_ = false;
}

} // namespace nightwalker::systems
