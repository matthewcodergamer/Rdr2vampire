#include "nightwalker/game/GameCombatApi.h"

#include <natives.h>

namespace nightwalker::game {
namespace {
Vec3 FromNative(const Vector3& value) noexcept {
    return {value.x, value.y, value.z};
}
}

Vec3 GameCombatApi::EntityVelocity(EntityHandle entity) const noexcept {
    if (entity == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(entity))) return {};
    return FromNative(ENTITY::GET_ENTITY_VELOCITY(static_cast<Entity>(entity), 0));
}

PedHandle GameCombatApi::PlayerAimedPed(PedHandle playerPed) const noexcept {
    if (playerPed == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(playerPed))) return 0;

    Entity aimedEntity = 0;
    const Player player = PLAYER::PLAYER_ID();
    if (!PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(player, &aimedEntity) || aimedEntity == 0) return 0;
    if (!ENTITY::DOES_ENTITY_EXIST(aimedEntity)) return 0;

    const Ped ped = ENTITY::GET_PED_INDEX_FROM_ENTITY_INDEX(aimedEntity);
    if (ped == 0 || ped == static_cast<Ped>(playerPed)) return 0;
    return static_cast<PedHandle>(ped);
}

bool GameCombatApi::IsPedInCombatWith(PedHandle ped, PedHandle target) const noexcept {
    if (ped == 0 || target == 0) return false;
    return PED::IS_PED_IN_COMBAT(static_cast<Ped>(ped), static_cast<Ped>(target));
}

bool GameCombatApi::IsPedInMeleeCombat(PedHandle ped) const noexcept {
    return ped != 0 && PED::IS_PED_IN_MELEE_COMBAT(static_cast<Ped>(ped));
}

void GameCombatApi::TaskStandStill(PedHandle ped, int durationMs) noexcept {
    if (ped == 0) return;
    TASK::TASK_STAND_STILL(static_cast<Ped>(ped), durationMs);
}

void GameCombatApi::TaskCombatPed(PedHandle ped, PedHandle target) noexcept {
    if (ped == 0 || target == 0) return;
    TASK::TASK_COMBAT_PED(static_cast<Ped>(ped), static_cast<Ped>(target), 0, 0);
}

void GameCombatApi::ClearPedTasks(PedHandle ped) noexcept {
    if (ped == 0) return;
    TASK::CLEAR_PED_TASKS(static_cast<Ped>(ped), TRUE, FALSE);
}

} // namespace nightwalker::game
