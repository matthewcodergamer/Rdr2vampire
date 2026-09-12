#pragma once

#include "nightwalker/game/GameApi.h"

namespace nightwalker::game {

class IGamePhysicalApi {
public:
    virtual ~IGamePhysicalApi() = default;
    [[nodiscard]] virtual bool WasContactFrom(EntityHandle target, EntityHandle source) const noexcept = 0;
    virtual void ClearContactSource(EntityHandle entity) noexcept = 0;
    [[nodiscard]] virtual bool CanRagdoll(PedHandle ped) const noexcept = 0;
    virtual bool SetRagdoll(PedHandle ped, int durationMs) noexcept = 0;
    virtual bool ApplyImpulse(EntityHandle entity, const Vec3& impulse) noexcept = 0;
};

class GamePhysicalApi final : public IGamePhysicalApi {
public:
    [[nodiscard]] bool WasContactFrom(EntityHandle target, EntityHandle source) const noexcept override;
    void ClearContactSource(EntityHandle entity) noexcept override;
    [[nodiscard]] bool CanRagdoll(PedHandle ped) const noexcept override;
    bool SetRagdoll(PedHandle ped, int durationMs) noexcept override;
    bool ApplyImpulse(EntityHandle entity, const Vec3& impulse) noexcept override;
};

} // namespace nightwalker::game
