#include "nightwalker/systems/SaintDenisDirector.h"

#include <algorithm>
#include "nightwalker/narrative/NarrativeScript.h"
#include "nightwalker/systems/EncounterMath.h"

namespace nightwalker::systems {

bool SaintDenisDirector::EnterCombat(std::uint64_t nowMs, std::string_view reason) noexcept {
    conversation_.SetEncounterConversationEnabled(false);
    pendingConversationIntent_ = narrative::ConversationIntent::None;
    if (!registry_.SetCombatEnabled(actor_, BossOwner::Encounter, true)) {
        BeginAbort("could not arm encounter combat ownership", nowMs);
        return false;
    }
    if (bossHud_.BeginBoss(actor_, config_.bossHud.displayName, nowMs)) bossHud_.NotifyCombatActivity(nowMs);
    logger_.Write(util::LogLevel::Info, std::string("Saint Denis vampire encounter entered combat: ") + std::string(reason));
    outsideSinceMs_ = 0;
    Transition(SaintDenisState::Combat, nowMs);
    return true;
}

void SaintDenisDirector::UpdateActive(const core::FrameContext& frame) {
    switch (state_) {
        case SaintDenisState::Stalking: {
            if (!ActorValid(false)) { BeginAbort("stalking actor became invalid", frame.nowMs); return; }
            if (!api_.PedAlive(actor_)) {
                resolvedThisSession_ = true; cleanupResolved_ = true;
                registry_.SetCombatEnabled(actor_, BossOwner::Encounter, false);
                narrative_.StartSequence(narrative::ids::kSaintDenisPostDefeat, frame.nowMs);
                Transition(SaintDenisState::Resolution, frame.nowMs); return;
            }
            if (!PreCombatStillSafe()) { BeginAbort("stalking became unsafe", frame.nowMs); return; }
            const auto player = api_.PlayerPed();
            const bool aimed = combatApi_.PlayerAimedPed(player) == actor_;
            const float distance = encounter_math::Distance2D(api_.EntityCoords(player), api_.EntityCoords(actor_));
            if (aimed || distance <= static_cast<float>(config_.encounter.confrontationDistance) ||
                frame.nowMs - stateStartedMs_ >= static_cast<std::uint64_t>(config_.encounter.stalkingMs)) {
                narrative_.StartSequenceFamily(narrative::ids::kSaintDenisPreFight, frame.nowMs);
                const int holdMs = std::max(config_.narrative.conversationWindowMs + 1000,
                    config_.encounter.confrontationMs + 1000);
                combatApi_.TaskStandStill(actor_, holdMs);
                presentationApi_.PlayShadowSmoke(api_.EntityCoords(actor_), 0.42F);
                pendingConversationIntent_ = narrative::ConversationIntent::None;
                conversation_.SetEncounterConversationEnabled(true);
                Transition(SaintDenisState::Confrontation, frame.nowMs);
            }
            return;
        }
        case SaintDenisState::Confrontation: {
            if (!ActorValid()) { BeginAbort("confrontation actor invalid", frame.nowMs); return; }
            if (!PreCombatStillSafe()) { BeginAbort("confrontation became unsafe", frame.nowMs); return; }

            const auto intent = conversation_.ConsumeIntent();
            if (intent == narrative::ConversationIntent::Hostile) {
                narrative_.Cancel();
                EnterCombat(frame.nowMs, "player fired during conversation");
                return;
            }
            if (intent == narrative::ConversationIntent::Challenge || intent == narrative::ConversationIntent::Leave)
                pendingConversationIntent_ = intent;

            if (pendingConversationIntent_ == narrative::ConversationIntent::Challenge && !narrative_.Active()) {
                EnterCombat(frame.nowMs, "player challenged the vampire");
                return;
            }
            if (pendingConversationIntent_ == narrative::ConversationIntent::Leave && !narrative_.Active()) {
                conversation_.SetEncounterConversationEnabled(false);
                BeginAbort("player chose to leave the confrontation", frame.nowMs);
                return;
            }

            const auto elapsed = frame.nowMs - stateStartedMs_;
            const bool initialDialogueDone = !narrative_.IsPlayingFamily(narrative::ids::kSaintDenisPreFight);
            if (initialDialogueDone && elapsed >= static_cast<std::uint64_t>(config_.narrative.conversationWindowMs) &&
                pendingConversationIntent_ == narrative::ConversationIntent::None && !narrative_.Active()) {
                EnterCombat(frame.nowMs, "conversation window expired");
            }
            return;
        }
        case SaintDenisState::Combat: {
            conversation_.SetEncounterConversationEnabled(false);
            if (!ActorValid(false)) { bossHud_.ForceHide(); BeginAbort("combat actor became invalid", frame.nowMs); return; }
            if (!api_.PedAlive(actor_)) {
                registry_.SetCombatEnabled(actor_, BossOwner::Encounter, false);
                bossHud_.EndBoss(true, frame.nowMs);
                narrative_.StartSequence(narrative::ids::kSaintDenisPostDefeat, frame.nowMs);
                resolvedThisSession_ = true; cleanupResolved_ = true;
                logger_.Write(util::LogLevel::Info, "Saint Denis vampire encounter resolved by boss death.");
                Transition(SaintDenisState::Resolution, frame.nowMs); return;
            }
            if (!config_.IsFeatureEnabled(core::Feature::Encounter) ||
                !config_.IsFeatureEnabled(core::Feature::VampireAi) ||
                !encounter_math::IsHourInWindow(encounterApi_.ClockHour(), config_.encounter.startHour, config_.encounter.endHour)) {
                BeginAbort("combat window became unsafe", frame.nowMs); return;
            }
            const game::Vec3 center{static_cast<float>(config_.encounter.centerX), static_cast<float>(config_.encounter.centerY), static_cast<float>(config_.encounter.centerZ)};
            const bool outside = !encounter_math::WithinRadius(api_.EntityCoords(api_.PlayerPed()), center, static_cast<float>(config_.encounter.abortRadius));
            if (outside) {
                if (outsideSinceMs_ == 0) outsideSinceMs_ = frame.nowMs;
                if (frame.nowMs - outsideSinceMs_ >= static_cast<std::uint64_t>(config_.encounter.leaveGraceMs)) BeginAbort("player remained outside encounter area", frame.nowMs);
            } else outsideSinceMs_ = 0;
            return;
        }
        default: return;
    }
}

} // namespace nightwalker::systems
