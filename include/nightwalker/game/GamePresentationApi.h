#pragma once
#include "nightwalker/game/GameApi.h"
namespace nightwalker::game {
class IGamePresentationApi {
public:
    virtual ~IGamePresentationApi() = default;
    virtual bool SetPedVisible(PedHandle ped, bool visible) noexcept = 0;
    virtual void RestorePedAppearance(PedHandle ped) noexcept = 0;
    virtual bool MeleeInputPressed() const noexcept = 0;
    virtual bool PulseMeleeInput() noexcept = 0;
    virtual void RequestShadowSmoke() noexcept = 0;
    virtual bool PlayShadowSmoke(const Vec3& position, float scale) noexcept = 0;
    virtual void ReleaseShadowSmoke() noexcept = 0;

    // Backwards-compatible aliases used by the existing player-side debug harness.
    bool SetPlayerVisible(PedHandle ped, bool visible) noexcept { return SetPedVisible(ped, visible); }
    void RestorePlayerAppearance(PedHandle ped) noexcept { RestorePedAppearance(ped); }
};
class GamePresentationApi final : public IGamePresentationApi {
public:
    bool SetPedVisible(PedHandle ped, bool visible) noexcept override;
    void RestorePedAppearance(PedHandle ped) noexcept override;
    bool MeleeInputPressed() const noexcept override;
    bool PulseMeleeInput() noexcept override;
    void RequestShadowSmoke() noexcept override;
    bool PlayShadowSmoke(const Vec3& position, float scale) noexcept override;
    void ReleaseShadowSmoke() noexcept override;
};
}
