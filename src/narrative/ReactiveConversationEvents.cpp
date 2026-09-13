#include "nightwalker/narrative/ReactiveConversationController.h"

namespace nightwalker::narrative {
namespace {
constexpr std::uint64_t kAmbientReactionCooldownMs = 1700;
constexpr std::string_view kQuestion = "saint_denis.choice.question";
constexpr std::string_view kChallenge = "saint_denis.choice.challenge";
constexpr std::string_view kLeave = "saint_denis.choice.leave";
constexpr std::string_view kAim = "saint_denis.react.aim";
constexpr std::string_view kAimHeld = "saint_denis.react.aim_hold";
constexpr std::string_view kLowered = "saint_denis.react.lowered";
constexpr std::string_view kShotHit = "saint_denis.react.shot_hit";
constexpr std::string_view kShotMiss = "saint_denis.react.shot_miss";
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
    switch (event) {
        case ReactiveDialogueEvent::Question:
            StartFamily(kQuestion, nowMs, false); return;
        case ReactiveDialogueEvent::Challenge:
            if (StartFamily(kChallenge, nowMs, false)) intent_ = ConversationIntent::Challenge;
            return;
        case ReactiveDialogueEvent::Leave:
            if (StartFamily(kLeave, nowMs, false)) intent_ = ConversationIntent::Leave;
            return;
        case ReactiveDialogueEvent::AimStarted:
            if (nowMs >= nextAmbientReactionMs_ && StartFamily(kAim, nowMs, false))
                nextAmbientReactionMs_ = nowMs + kAmbientReactionCooldownMs;
            return;
        case ReactiveDialogueEvent::AimHeld:
            if (nowMs >= nextAmbientReactionMs_ && StartFamily(kAimHeld, nowMs, false))
                nextAmbientReactionMs_ = nowMs + kAmbientReactionCooldownMs;
            return;
        case ReactiveDialogueEvent::AimLowered:
            if (nowMs >= nextAmbientReactionMs_ && StartFamily(kLowered, nowMs, false))
                nextAmbientReactionMs_ = nowMs + kAmbientReactionCooldownMs;
            return;
        case ReactiveDialogueEvent::ShotStarted:
            SetPrompts(false);
            if (narrative_.Active()) narrative_.Cancel();
            intent_ = ConversationIntent::Hostile;
            return;
        case ReactiveDialogueEvent::ShotHit:
            physicalApi_.ClearContactSource(actor_);
            StartFamily(kShotHit, nowMs, true); return;
        case ReactiveDialogueEvent::ShotMiss:
            StartFamily(kShotMiss, nowMs, true); return;
        default:
            return;
    }
}

} // namespace nightwalker::narrative
