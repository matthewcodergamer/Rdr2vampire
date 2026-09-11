#include "nightwalker/game/GameApi.h"

#include <natives.h>

namespace nightwalker::game {
namespace {

constexpr int kTraceWorld = 1;
constexpr int kTraceVehicles = 2;
constexpr int kTraceObjects = 16;
constexpr int kTraceFlags = kTraceWorld | kTraceVehicles | kTraceObjects;
constexpr int kTraceOptions = 7;

Vec3 FromNative(const Vector3& value) noexcept {
    return {value.x, value.y, value.z};
}

} // namespace

PedHandle GameApi::PlayerPed() const noexcept {
    return static_cast<PedHandle>(PLAYER::PLAYER_PED_ID());
}

bool GameApi::EntityExists(EntityHandle entity) const noexcept {
    return entity != 0 && ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(entity));
}

bool GameApi::PedAlive(PedHandle ped) const noexcept {
    return EntityExists(ped) && !ENTITY::IS_ENTITY_DEAD(static_cast<Entity>(ped));
}

ModelHash GameApi::EntityModel(EntityHandle entity) const noexcept {
    return EntityExists(entity)
        ? static_cast<ModelHash>(ENTITY::GET_ENTITY_MODEL(static_cast<Entity>(entity)))
        : 0;
}

Vec3 GameApi::EntityCoords(EntityHandle entity) const noexcept {
    if (!EntityExists(entity)) return {};
    return FromNative(ENTITY::GET_ENTITY_COORDS(static_cast<Entity>(entity), TRUE, TRUE));
}

float GameApi::EntityHeading(EntityHandle entity) const noexcept {
    return EntityExists(entity) ? ENTITY::GET_ENTITY_HEADING(static_cast<Entity>(entity)) : 0.0F;
}

Vec3 GameApi::EntityForward(EntityHandle entity) const noexcept {
    if (!EntityExists(entity)) return {};
    return FromNative(ENTITY::GET_ENTITY_FORWARD_VECTOR(static_cast<Entity>(entity)));
}

Vec3 GameApi::OffsetFromEntity(EntityHandle entity, float x, float y, float z) const noexcept {
    if (!EntityExists(entity)) return {};
    return FromNative(ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(
        static_cast<Entity>(entity), x, y, z));
}

bool GameApi::FindSafeCoordForPed(const Vec3& nearPosition, Vec3& safePosition) const noexcept {
    Vector3 output{};
    const bool found = PATHFIND::GET_SAFE_COORD_FOR_PED(
        nearPosition.x, nearPosition.y, nearPosition.z, FALSE, &output, 0);
    if (!found) return false;
    safePosition = FromNative(output);
    return true;
}

bool GameApi::TryGroundZ(const Vec3& position, float probeHeight, float& groundZ) const noexcept {
    groundZ = 0.0F;
    return MISC::GET_GROUND_Z_FOR_3D_COORD(
        position.x, position.y, position.z + probeHeight, &groundZ, FALSE);
}

bool GameApi::HasWaterAt(const Vec3& position, float& waterHeight) const noexcept {
    waterHeight = 0.0F;
    return WATER::GET_WATER_HEIGHT_NO_WAVES(
        position.x, position.y, position.z + 2.0F, &waterHeight);
}

RaycastResult GameApi::RaycastWorld(
    const Vec3& start,
    const Vec3& end,
    EntityHandle entityToIgnore) const noexcept {
    RaycastResult result{};
    const ScrHandle handle = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(
        start.x, start.y, start.z,
        end.x, end.y, end.z,
        kTraceFlags,
        static_cast<Entity>(entityToIgnore),
        kTraceOptions);
    if (handle == 0) return result;

    BOOL hit = FALSE;
    Vector3 endCoords{};
    Vector3 surfaceNormal{};
    Entity entityHit = 0;
    const int status = SHAPETEST::GET_SHAPE_TEST_RESULT(
        handle, &hit, &endCoords, &surfaceNormal, &entityHit);
    if (status != 2) return result;

    result.conclusive = true;
    result.hit = hit == TRUE;
    result.endCoords = FromNative(endCoords);
    result.surfaceNormal = FromNative(surfaceNormal);
    result.entityHit = static_cast<EntityHandle>(entityHit);
    return result;
}

bool GameApi::SetEntityCoordsNoOffset(EntityHandle entity, const Vec3& position) noexcept {
    if (!EntityExists(entity)) return false;
    ENTITY::SET_ENTITY_COORDS_NO_OFFSET(
        static_cast<Entity>(entity), position.x, position.y, position.z,
        FALSE, FALSE, TRUE);
    return EntityExists(entity);
}

bool GameApi::IsPedModelAvailable(ModelHash model) const noexcept {
    const Hash nativeModel = static_cast<Hash>(model);
    return model != 0 && STREAMING::IS_MODEL_IN_CDIMAGE(nativeModel) &&
           STREAMING::IS_MODEL_VALID(nativeModel) && STREAMING::IS_MODEL_A_PED(nativeModel);
}

void GameApi::RequestModel(ModelHash model) noexcept {
    STREAMING::REQUEST_MODEL(static_cast<Hash>(model), FALSE);
}

bool GameApi::IsModelLoaded(ModelHash model) const noexcept {
    return STREAMING::HAS_MODEL_LOADED(static_cast<Hash>(model));
}

void GameApi::ReleaseModel(ModelHash model) noexcept {
    if (model != 0) STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(static_cast<Hash>(model));
}

PedHandle GameApi::CreateLocalPed(ModelHash model, const Vec3& position, float heading) noexcept {
    return static_cast<PedHandle>(PED::CREATE_PED(
        static_cast<Hash>(model), position.x, position.y, position.z, heading,
        FALSE, FALSE, 0, FALSE));
}

bool GameApi::DeletePed(PedHandle& ped) noexcept {
    if (ped == 0) return true;
    if (!EntityExists(ped)) {
        ped = 0;
        return true;
    }

    Ped nativePed = static_cast<Ped>(ped);
    PED::DELETE_PED(&nativePed);
    ped = static_cast<PedHandle>(nativePed);

    if (ped == 0 || !EntityExists(ped)) {
        ped = 0;
        return true;
    }
    return false;
}

} // namespace nightwalker::game
