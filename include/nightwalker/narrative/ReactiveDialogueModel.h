#pragma once

#include <cstdint>

#include "nightwalker/game/GameConversationApi.h"

namespace nightwalker::narrative {

enum class ReactiveDialogueEvent {
    None,
    Question,
    Challenge,
    Leave,
    AimStarted,
    AimHeld,
    AimLowered,
    ShotStarted,
    ShotHit,
    ShotMiss,
    MeleeWeaponDrawn,
    RangedWeaponDrawn,
    LassoDrawn,
    ThrowableDrawn,
    WeaponPutAway,
    UnarmedAttackStarted,
    MeleeAttackStarted,
    UnarmedHit,
    MeleeHit,
    CloseApproach,
    BackedAway
};

struct ReactiveDialogueInput final {
    std::uint64_t nowMs{0};
    game::PlayerWeaponKind weaponKind{game::PlayerWeaponKind::Unknown};
    float distanceToBoss{-1.0F};
    bool aimed{false};
    bool shooting{false};
    bool hitBoss{false};
    bool meleeEngaged{false};
    bool question{false};
    bool challenge{false};
    bool leave{false};
};

class ReactiveDialogueModel final {
public:
    ReactiveDialogueEvent Update(const ReactiveDialogueInput& input) noexcept;
    void Reset() noexcept;
    [[nodiscard]] bool ShotPending() const noexcept { return shotPending_; }

private:
    game::PlayerWeaponKind weaponKind_{game::PlayerWeaponKind::Unknown};
    bool weaponInitialized_{false};
    bool aimed_{false};
    bool shooting_{false};
    bool aimHeldEmitted_{false};
    bool shotPending_{false};
    bool meleeEngaged_{false};
    bool meleeHitLatched_{false};
    bool proximityInitialized_{false};
    bool closeRange_{false};
    std::uint64_t aimStartedMs_{0};
    std::uint64_t shotDeadlineMs_{0};
};

} // namespace nightwalker::narrative
