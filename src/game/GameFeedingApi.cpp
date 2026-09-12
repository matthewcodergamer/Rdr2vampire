#include "nightwalker/game/GameFeedingApi.h"
#include <algorithm>
#include <natives.h>

namespace nightwalker::game {

bool GameFeedingApi::IsHuman(PedHandle ped) const noexcept {
    return ped != 0 && ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped)) &&
           PED::IS_PED_HUMAN(static_cast<Ped>(ped)) == TRUE;
}

bool GameFeedingApi::IsMissionEntity(EntityHandle entity) const noexcept {
    return entity != 0 && ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(entity)) &&
           ENTITY::IS_ENTITY_A_MISSION_ENTITY(static_cast<Entity>(entity)) == TRUE;
}

bool GameFeedingApi::IsPedRestricted(PedHandle ped) const noexcept {
    if (ped == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped))) return true;
    const auto value = static_cast<Ped>(ped);
    return PED::IS_PED_RAGDOLL(value) == TRUE ||
           PED::IS_PED_FALLING(value) == TRUE ||
           PED::IS_PED_SWIMMING(value) == TRUE ||
           PED::IS_PED_ON_MOUNT(value) == TRUE ||
           PED::IS_PED_IN_ANY_VEHICLE(value, FALSE) == TRUE ||
           PED::IS_PED_USING_ANY_SCENARIO(value) == TRUE;
}

bool GameFeedingApi::HasClearLos(EntityHandle from, EntityHandle to) const noexcept {
    if (from == 0 || to == 0) return false;
    if (!ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(from)) ||
        !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(to))) return false;
    return ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(static_cast<Entity>(from), static_cast<Entity>(to), 16) == TRUE;
}

int GameFeedingApi::Health(PedHandle ped) const noexcept {
    if (ped == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped))) return 0;
    return ENTITY::GET_ENTITY_HEALTH(static_cast<Entity>(ped));
}

int GameFeedingApi::MaxHealth(PedHandle ped) const noexcept {
    if (ped == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped))) return 0;
    return PED::GET_PED_MAX_HEALTH(static_cast<Ped>(ped));
}

bool GameFeedingApi::SetHealth(PedHandle ped, int health) noexcept {
    if (ped == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped))) return false;
    const int maxHealth = std::max(0, PED::GET_PED_MAX_HEALTH(static_cast<Ped>(ped)));
    const int clamped = std::clamp(health, 0, maxHealth);
    ENTITY::SET_ENTITY_HEALTH(static_cast<Entity>(ped), clamped, 0);
    return true;
}

bool GameFeedingApi::FacePedToward(PedHandle ped, PedHandle target) noexcept {
    if (ped == 0 || target == 0) return false;
    if (!ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped)) ||
        !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(target))) return false;
    TASK::TASK_TURN_PED_TO_FACE_ENTITY(static_cast<Ped>(ped), static_cast<Entity>(target), 300, 0.0F, 0.0F, 0.0F);
    return true;
}

void GameFeedingApi::StandStill(PedHandle ped, int durationMs) noexcept {
    if (ped == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped))) return;
    TASK::TASK_STAND_STILL(static_cast<Ped>(ped), std::max(durationMs, 0));
}

bool GameFeedingApi::StartGrapple(PedHandle attacker, PedHandle target) noexcept {
    if (attacker == 0 || target == 0) return false;
    if (!ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(attacker)) ||
        !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(target))) return false;
    return TASK::TASK_GRAPPLE(static_cast<Ped>(attacker), static_cast<Ped>(target), 0, TRUE, 1.0F, TRUE, 0) == TRUE;
}

void GameFeedingApi::ClearTasks(PedHandle ped) noexcept {
    if (ped == 0 || !ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped))) return;
    TASK::CLEAR_PED_TASKS(static_cast<Ped>(ped), TRUE, FALSE);
}

} // namespace nightwalker::game
