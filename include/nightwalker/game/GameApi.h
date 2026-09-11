#pragma once

#include <cstdint>

namespace nightwalker::game {

using EntityHandle = int;
using PedHandle = int;
using ModelHash = std::uint32_t;

struct Vec3 final {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct RaycastResult final {
    bool conclusive{false};
    bool hit{false};
    Vec3 endCoords{};
    Vec3 surfaceNormal{};
    EntityHandle entityHit{0};
};

class IGameApi {
public:
    virtual ~IGameApi() = default;

    virtual PedHandle PlayerPed() const noexcept = 0;
    virtual bool EntityExists(EntityHandle entity) const noexcept = 0;
    virtual bool PedAlive(PedHandle ped) const noexcept = 0;
    virtual ModelHash EntityModel(EntityHandle entity) const noexcept = 0;
    virtual Vec3 EntityCoords(EntityHandle entity) const noexcept = 0;
    virtual float EntityHeading(EntityHandle entity) const noexcept = 0;
    virtual Vec3 EntityForward(EntityHandle) const noexcept { return {}; }
    virtual Vec3 OffsetFromEntity(EntityHandle entity, float x, float y, float z) const noexcept = 0;
    virtual bool FindSafeCoordForPed(const Vec3& nearPosition, Vec3& safePosition) const noexcept = 0;
    virtual bool TryGroundZ(const Vec3&, float, float&) const noexcept { return false; }
    virtual bool HasWaterAt(const Vec3& position, float& waterHeight) const noexcept = 0;
    virtual RaycastResult RaycastWorld(const Vec3&, const Vec3&, EntityHandle) const noexcept { return {}; }
    virtual bool SetEntityCoordsNoOffset(EntityHandle, const Vec3&) noexcept { return false; }

    virtual bool IsPedModelAvailable(ModelHash model) const noexcept = 0;
    virtual void RequestModel(ModelHash model) noexcept = 0;
    virtual bool IsModelLoaded(ModelHash model) const noexcept = 0;
    virtual void ReleaseModel(ModelHash model) noexcept = 0;

    virtual PedHandle CreateLocalPed(ModelHash model, const Vec3& position, float heading) noexcept = 0;
    virtual bool DeletePed(PedHandle& ped) noexcept = 0;
};

class GameApi final : public IGameApi {
public:
    PedHandle PlayerPed() const noexcept override;
    bool EntityExists(EntityHandle entity) const noexcept override;
    bool PedAlive(PedHandle ped) const noexcept override;
    ModelHash EntityModel(EntityHandle entity) const noexcept override;
    Vec3 EntityCoords(EntityHandle entity) const noexcept override;
    float EntityHeading(EntityHandle entity) const noexcept override;
    Vec3 EntityForward(EntityHandle entity) const noexcept override;
    Vec3 OffsetFromEntity(EntityHandle entity, float x, float y, float z) const noexcept override;
    bool FindSafeCoordForPed(const Vec3& nearPosition, Vec3& safePosition) const noexcept override;
    bool TryGroundZ(const Vec3& position, float probeHeight, float& groundZ) const noexcept override;
    bool HasWaterAt(const Vec3& position, float& waterHeight) const noexcept override;
    RaycastResult RaycastWorld(const Vec3& start, const Vec3& end,
                               EntityHandle entityToIgnore) const noexcept override;
    bool SetEntityCoordsNoOffset(EntityHandle entity, const Vec3& position) noexcept override;

    bool IsPedModelAvailable(ModelHash model) const noexcept override;
    void RequestModel(ModelHash model) noexcept override;
    bool IsModelLoaded(ModelHash model) const noexcept override;
    void ReleaseModel(ModelHash model) noexcept override;

    PedHandle CreateLocalPed(ModelHash model, const Vec3& position, float heading) noexcept override;
    bool DeletePed(PedHandle& ped) noexcept override;
};

} // namespace nightwalker::game
