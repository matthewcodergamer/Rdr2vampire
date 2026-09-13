#pragma once

#include <cstdint>

#include "nightwalker/game/GameConversationApi.h"

namespace nightwalker::narrative {

enum class ReactiveDialogueEvent {
    None, Question, Challenge, Leave,
    AimStarted, AimHeld, AimLowered,
    ShotStarted, ShotHit, ShotMiss,
    MeleeWeaponDrawn, RangedWeaponDrawn, LassoDrawn, ThrowableDrawn, WeaponPutAway,
    UnarmedAttackStarted, MeleeAttackStarted, UnarmedHit, MeleeHit,
    CloseApproach, BackedAway
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
    ReactiveDialogueEvent Update(const ReactiveDialogueInput& in) noexcept {
        constexpr std::uint64_t kAimHoldMs = 2200;
        constexpr std::uint64_t kShotClassifyMs = 320;
        constexpr float kCloseApproachMeters = 2.6F;
        constexpr float kBackAwayMeters = 7.5F;

        if (in.hitBoss && shotPending_) {
            shotPending_ = false; shooting_ = in.shooting; aimed_ = in.aimed;
            return ReactiveDialogueEvent::ShotHit;
        }
        if (in.shooting && !shooting_) {
            shooting_ = true; shotPending_ = true; shotDeadlineMs_ = in.nowMs + kShotClassifyMs; aimed_ = in.aimed;
            return ReactiveDialogueEvent::ShotStarted;
        }
        if (!in.shooting) shooting_ = false;
        if (shotPending_ && in.nowMs >= shotDeadlineMs_) {
            shotPending_ = false;
            return in.hitBoss ? ReactiveDialogueEvent::ShotHit : ReactiveDialogueEvent::ShotMiss;
        }

        if (!in.hitBoss) meleeHitLatched_ = false;
        if (in.meleeEngaged && !meleeEngaged_) {
            meleeEngaged_ = true; meleeHitLatched_ = in.hitBoss;
            if (in.weaponKind == game::PlayerWeaponKind::Unarmed) return ReactiveDialogueEvent::UnarmedAttackStarted;
            if (in.weaponKind == game::PlayerWeaponKind::Melee) return ReactiveDialogueEvent::MeleeAttackStarted;
        }
        if (!in.meleeEngaged) meleeEngaged_ = false;
        if (in.meleeEngaged && in.hitBoss && !meleeHitLatched_) {
            meleeHitLatched_ = true;
            if (in.weaponKind == game::PlayerWeaponKind::Unarmed) return ReactiveDialogueEvent::UnarmedHit;
            if (in.weaponKind == game::PlayerWeaponKind::Melee) return ReactiveDialogueEvent::MeleeHit;
        }

        if (in.question) return ReactiveDialogueEvent::Question;
        if (in.challenge) return ReactiveDialogueEvent::Challenge;
        if (in.leave) return ReactiveDialogueEvent::Leave;

        if (!weaponInitialized_) { weaponInitialized_ = true; weaponKind_ = in.weaponKind; }
        else if (in.weaponKind != game::PlayerWeaponKind::Unknown && in.weaponKind != weaponKind_) {
            const auto previous = weaponKind_; weaponKind_ = in.weaponKind;
            if (in.weaponKind == game::PlayerWeaponKind::Unarmed && previous != game::PlayerWeaponKind::Unknown && previous != game::PlayerWeaponKind::Unarmed)
                return ReactiveDialogueEvent::WeaponPutAway;
            switch (in.weaponKind) {
                case game::PlayerWeaponKind::Melee: return ReactiveDialogueEvent::MeleeWeaponDrawn;
                case game::PlayerWeaponKind::Lasso: return ReactiveDialogueEvent::LassoDrawn;
                case game::PlayerWeaponKind::Thrown: return ReactiveDialogueEvent::ThrowableDrawn;
                case game::PlayerWeaponKind::Ranged: return ReactiveDialogueEvent::RangedWeaponDrawn;
                default: break;
            }
        }

        if (in.aimed && !aimed_) { aimed_ = true; aimHeldEmitted_ = false; aimStartedMs_ = in.nowMs; return ReactiveDialogueEvent::AimStarted; }
        if (in.aimed && aimed_ && !aimHeldEmitted_ && in.nowMs - aimStartedMs_ >= kAimHoldMs) { aimHeldEmitted_ = true; return ReactiveDialogueEvent::AimHeld; }
        if (!in.aimed && aimed_) { aimed_ = false; aimHeldEmitted_ = false; aimStartedMs_ = 0; return ReactiveDialogueEvent::AimLowered; }

        if (in.distanceToBoss >= 0.0F) {
            if (!proximityInitialized_) { proximityInitialized_ = true; closeRange_ = in.distanceToBoss <= kCloseApproachMeters; }
            else if (!closeRange_ && in.distanceToBoss <= kCloseApproachMeters) { closeRange_ = true; return ReactiveDialogueEvent::CloseApproach; }
            else if (closeRange_ && in.distanceToBoss >= kBackAwayMeters) { closeRange_ = false; return ReactiveDialogueEvent::BackedAway; }
        }
        return ReactiveDialogueEvent::None;
    }

    void Reset() noexcept {
        weaponKind_ = game::PlayerWeaponKind::Unknown; weaponInitialized_ = false;
        aimed_ = false; shooting_ = false; aimHeldEmitted_ = false; shotPending_ = false;
        meleeEngaged_ = false; meleeHitLatched_ = false; proximityInitialized_ = false; closeRange_ = false;
        aimStartedMs_ = 0; shotDeadlineMs_ = 0;
    }

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
