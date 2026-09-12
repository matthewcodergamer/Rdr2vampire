#pragma once

#include "nightwalker/game/GameApi.h"

namespace nightwalker::game {

class IGameMovementApi {
public:
    virtual ~IGameMovementApi() = default;

    virtual bool SetMoveRate(PedHandle ped, float multiplier) noexcept = 0;
    [[nodiscard]] virtual bool IsSwimming(PedHandle ped) const noexcept = 0;
    [[nodiscard]] virtual bool IsRagdoll(PedHandle ped) const noexcept = 0;
    [[nodiscard]] virtual bool IsFalling(PedHandle ped) const noexcept = 0;
    [[nodiscard]] virtual bool IsOnMount(PedHandle ped) const noexcept = 0;
};

class GameMovementApi final : public IGameMovementApi {
public:
    bool SetMoveRate(PedHandle ped, float multiplier) noexcept override;
    [[nodiscard]] bool IsSwimming(PedHandle ped) const noexcept override;
    [[nodiscard]] bool IsRagdoll(PedHandle ped) const noexcept override;
    [[nodiscard]] bool IsFalling(PedHandle ped) const noexcept override;
    [[nodiscard]] bool IsOnMount(PedHandle ped) const noexcept override;
};

} // namespace nightwalker::game
