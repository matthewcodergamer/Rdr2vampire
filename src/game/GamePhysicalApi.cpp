#include "nightwalker/game/GamePhysicalApi.h"

#include <algorithm>
#include <natives.h>

namespace nightwalker::game {

bool GamePhysicalApi::WasContactFrom(EntityHandle target, EntityHandle source) const noexcept {
    if (target == 0 || source == 0) return false;
    if (!ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(target)) ||
        !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(source))) return false;
    return ENTITY::HAS_ENTITY_BEEN_DAMAGED_BY_ENTITY(
        static_cast<Entity>(target), static_cast<Entity>(source), TRUE, TRUE) == TRUE;
}

void GamePhysicalApi::ClearContactSource(EntityHandle entity) noexcept {
    if (entity == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(entity))) return;
    ENTITY::CLEAR_ENTITY_LAST_DAMAGE_ENTITY(static_cast<Entity>(entity));
}

bool GamePhysicalApi::SetRagdoll(PedHandle ped, int durationMs) noexcept {
    if (ped == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped))) return false;
    const int duration = std::clamp(durationMs, 100, 5000);
    return PED::SET_PED_TO_RAGDOLL(static_cast<Ped>(ped), duration, duration, 0, TRUE, TRUE, FALSE) == TRUE;
}

bool GamePhysicalApi::ApplyImpulse(EntityHandle entity, const Vec3& impulse) noexcept {
    if (entity == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(entity))) return false;
    ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
        static_cast<Entity>(entity),
        1,
        impulse.x,
        impulse.y,
        impulse.z,
        FALSE,
        FALSE,
        FALSE,
        FALSE);
    return true;
}

} // namespace nightwalker::game
