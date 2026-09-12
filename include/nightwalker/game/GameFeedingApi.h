#pragma once

#include "nightwalker/game/GameApi.h"

namespace nightwalker::game {

class IGameFeedingApi {
public:
    virtual ~IGameFeedingApi() = default;
    virtual bool IsHuman(PedHandle ped) const noexcept = 0;
    virtual bool IsMissionEntity(EntityHandle entity) const noexcept = 0;
    virtual bool IsPedRestricted(PedHandle ped) const noexcept = 0;
    virtual bool HasClearLos(EntityHandle from, EntityHandle to) const noexcept = 0;
    virtual int Health(PedHandle ped) const noexcept = 0;
    virtual int MaxHealth(PedHandle ped) const noexcept = 0;
    virtual bool SetHealth(PedHandle ped, int health) noexcept = 0;
    virtual bool FacePedToward(PedHandle ped, PedHandle target) noexcept = 0;
    virtual void StandStill(PedHandle ped, int durationMs) noexcept = 0;
    virtual bool StartGrapple(PedHandle attacker, PedHandle target) noexcept = 0;
    virtual void ClearTasks(PedHandle ped) noexcept = 0;
};

class GameFeedingApi final : public IGameFeedingApi {
public:
    bool IsHuman(PedHandle ped) const noexcept override;
    bool IsMissionEntity(EntityHandle entity) const noexcept override;
    bool IsPedRestricted(PedHandle ped) const noexcept override;
    bool HasClearLos(EntityHandle from, EntityHandle to) const noexcept override;
    int Health(PedHandle ped) const noexcept override;
    int MaxHealth(PedHandle ped) const noexcept override;
    bool SetHealth(PedHandle ped, int health) noexcept override;
    bool FacePedToward(PedHandle ped, PedHandle target) noexcept override;
    void StandStill(PedHandle ped, int durationMs) noexcept override;
    bool StartGrapple(PedHandle attacker, PedHandle target) noexcept override;
    void ClearTasks(PedHandle ped) noexcept override;
};

} // namespace nightwalker::game
