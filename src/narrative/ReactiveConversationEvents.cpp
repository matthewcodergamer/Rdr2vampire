#include "nightwalker/narrative/ReactiveConversationController.h"

namespace nightwalker::narrative {
namespace {
constexpr std::uint64_t kAmbientReactionCooldownMs = 1700;
constexpr std::uint64_t kWeaponReactionCooldownMs = 2200;
constexpr std::uint64_t kMovementReactionCooldownMs = 3000;
constexpr std::string_view kQuestion = "saint_denis.choice.question";
constexpr std::string_view kChallenge = "saint_denis.choice.challenge";
constexpr std::string_view kLeave = "saint_denis.choice.leave";
constexpr std::string_view kAim = "saint_denis.react.aim";
constexpr std::string_view kAimHeld = "saint_denis.react.aim_hold";
constexpr std::string_view kLowered = "saint_denis.react.lowered";
constexpr std::string_view kShotHit = "saint_denis.react.shot_hit";
constexpr std::string_view kShotMiss = "saint_denis.react.shot_miss";
constexpr std::string_view kMeleeDraw = "saint_denis.react.melee_draw";
constexpr std::string_view kRangedDraw = "saint_denis.react.ranged_draw";
constexpr std::string_view kLassoDraw = "saint_denis.react.lasso_draw";
constexpr std::string_view kThrownDraw = "saint_denis.react.thrown_draw";
constexpr std::string_view kHolster = "saint_denis.react.holster";
constexpr std::string_view kUnarmedStart = "saint_denis.react.unarmed_start";
constexpr std::string_view kMeleeStart = "saint_denis.react.melee_start";
constexpr std::string_view kUnarmedHit = "saint_denis.react.unarmed_hit";
constexpr std::string_view kMeleeHit = "saint_denis.react.melee_hit";
constexpr std::string_view kApproach = "saint_denis.react.approach";
constexpr std::string_view kBackAway = "saint_denis.react.back_away";
}

bool ReactiveConversationController::StartFamily(std::string_view family, std::uint64_t nowMs,
                                                 bool interrupt) noexcept {
    if (narrative_.Active()) {
        if (!interrupt) return false;
        narrative_.Cancel();
    }
    return narrative_.StartSequenceFamily(family, nowMs);
}

void ReactiveConversationController::HandleEvent(ReactiveDialogueEvent event,
                                                  std::uint64_t nowMs) noexcept {
    auto ambient = [&](std::string_view family, std::uint64_t cooldown) {
        if (nowMs >= nextAmbientReactionMs_ && StartFamily(family, nowMs, true))
            nextAmbientReactionMs_ = nowMs + cooldown;
    };

    switch (event) {
        case ReactiveDialogueEvent::Question: {
            const auto family = narrative_.HasSequenceFamily(ids::kSaintDenisRecordedQuestion)
                ? ids::kSaintDenisRecordedQuestion
                : kQuestion;
            StartFamily(family, nowMs, false);
            return;
        }
        case ReactiveDialogueEvent::Challenge:
            StartFamily(kChallenge, nowMs, false); intent_ = ConversationIntent::Challenge; return;
        case ReactiveDialogueEvent::Leave:
            StartFamily(kLeave, nowMs, false); intent_ = ConversationIntent::Leave; return;
        case ReactiveDialogueEvent::AimStarted:
            ambient(kAim, kAmbientReactionCooldownMs); return;
        case ReactiveDialogueEvent::AimHeld:
            ambient(kAimHeld, kAmbientReactionCooldownMs); return;
        case ReactiveDialogueEvent::AimLowered:
            ambient(kLowered, kAmbientReactionCooldownMs); return;
        case ReactiveDialogueEvent::MeleeWeaponDrawn:
            ambient(kMeleeDraw, kWeaponReactionCooldownMs); return;
        case ReactiveDialogueEvent::RangedWeaponDrawn:
            ambient(kRangedDraw, kWeaponReactionCooldownMs); return;
        case ReactiveDialogueEvent::LassoDrawn:
            ambient(kLassoDraw, kWeaponReactionCooldownMs); return;
        case ReactiveDialogueEvent::ThrowableDrawn:
            ambient(kThrownDraw, kWeaponReactionCooldownMs); return;
        case ReactiveDialogueEvent::WeaponPutAway:
            ambient(kHolster, kWeaponReactionCooldownMs); return;
        case ReactiveDialogueEvent::CloseApproach:
            ambient(kApproach, kMovementReactionCooldownMs); return;
        case ReactiveDialogueEvent::BackedAway:
            ambient(kBackAway, kMovementReactionCooldownMs); return;
        case ReactiveDialogueEvent::ShotStarted:
            SetPrompts(false);
            if (narrative_.Active()) narrative_.Cancel();
            intent_ = ConversationIntent::Hostile;
            return;
        case ReactiveDialogueEvent::UnarmedAttackStarted:
            SetPrompts(false);
            StartFamily(kUnarmedStart, nowMs, true);
            intent_ = ConversationIntent::Hostile;
            return;
        case ReactiveDialogueEvent::MeleeAttackStarted:
            SetPrompts(false);
            StartFamily(kMeleeStart, nowMs, true);
            intent_ = ConversationIntent::Hostile;
            return;
        case ReactiveDialogueEvent::ShotHit:
            physicalApi_.ClearContactSource(actor_);
            StartFamily(kShotHit, nowMs, true); return;
        case ReactiveDialogueEvent::ShotMiss:
            StartFamily(kShotMiss, nowMs, true); return;
        case ReactiveDialogueEvent::UnarmedHit:
            physicalApi_.ClearContactSource(actor_);
            ambient(kUnarmedHit, kAmbientReactionCooldownMs); return;
        case ReactiveDialogueEvent::MeleeHit:
            physicalApi_.ClearContactSource(actor_);
            ambient(kMeleeHit, kAmbientReactionCooldownMs); return;
        default:
            return;
    }
}

} // namespace nightwalker::narrative
