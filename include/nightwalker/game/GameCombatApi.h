#pragma once

#include "nightwalker/game/GameApi.h"

namespace nightwalker::game {

class IGameCombatApi {
public:
    virtual ~IGameCombatApi() = default;

    virtual Vec3 EntityVelocity(EntityHandle entity) const noexcept = 0;
    virtual PedHandle PlayerAimedPed(PedHandle playerPed) const noexcept = 0;
    virtual bool IsPedInCombatWith(PedHandle ped, PedHandle target) const noexcept = 0;
    virtual bool IsPedInMeleeCombat(PedHandle ped) const noexcept = 0;

    virtual void TaskStandStill(PedHandle ped, int durationMs) noexcept = 0;
    virtual void TaskFaceEntity(PedHandle ped, EntityHandle target, int durationMs) noexcept = 0;
    virtual void TaskCombatPed(PedHandle ped, PedHandle target) noexcept = 0;
    virtual void ClearPedTasks(PedHandle ped) noexcept = 0;
};

class GameCombatApi final : public IGameCombatApi {
public:
    Vec3 EntityVelocity(EntityHandle entity) const noexcept override;
    PedHandle PlayerAimedPed(PedHandle playerPed) const noexcept override;
    bool IsPedInCombatWith(PedHandle ped, PedHandle target) const noexcept override;
    bool IsPedInMeleeCombat(PedHandle ped) const noexcept override;

    void TaskStandStill(PedHandle ped, int durationMs) noexcept override;
    void TaskFaceEntity(PedHandle ped, EntityHandle target, int durationMs) noexcept override;
    void TaskCombatPed(PedHandle ped, PedHandle target) noexcept override;
    void ClearPedTasks(PedHandle ped) noexcept override;
};

} // namespace nightwalker::game
