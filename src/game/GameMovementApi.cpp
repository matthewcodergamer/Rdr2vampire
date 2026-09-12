#include "nightwalker/game/GameMovementApi.h"

#include <algorithm>
#include <natives.h>

namespace nightwalker::game {

bool GameMovementApi::SetMoveRate(PedHandle ped, float multiplier) noexcept {
    if (ped == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped))) return false;
    const float safe = std::clamp(multiplier, 0.0F, 2.0F);
    PED::SET_PED_MOVE_RATE_OVERRIDE(static_cast<Ped>(ped), safe);
    return true;
}

bool GameMovementApi::IsSwimming(PedHandle ped) const noexcept {
    return ped != 0 && PED::IS_PED_SWIMMING(static_cast<Ped>(ped)) == TRUE;
}

bool GameMovementApi::IsRagdoll(PedHandle ped) const noexcept {
    return ped != 0 && PED::IS_PED_RAGDOLL(static_cast<Ped>(ped)) == TRUE;
}

bool GameMovementApi::IsFalling(PedHandle ped) const noexcept {
    return ped != 0 && PED::IS_PED_FALLING(static_cast<Ped>(ped)) == TRUE;
}

bool GameMovementApi::IsOnMount(PedHandle ped) const noexcept {
    return ped != 0 && PED::IS_PED_ON_MOUNT(static_cast<Ped>(ped)) == TRUE;
}

} // namespace nightwalker::game
