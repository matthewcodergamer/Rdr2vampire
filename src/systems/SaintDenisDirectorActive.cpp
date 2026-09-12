#include "nightwalker/systems/SaintDenisDirector.h"

#include "nightwalker/systems/EncounterMath.h"

namespace nightwalker::systems {

void SaintDenisDirector::UpdateActive(const core::FrameContext& frame) {
    switch (state_) {
        case SaintDenisState::Stalking: {
            if (!ActorValid(false)) { BeginAbort("stalking actor became invalid", frame.nowMs); return; }
            if (!api_.PedAlive(actor_)) {
                resolvedThisSession_ = true; cleanupResolved_ = true;
                registry_.SetCombatEnabled(actor_, BossOwner::Encounter, false);
                Transition(SaintDenisState::Resolution, frame.nowMs); return;
            }
            if (!PreCombatStillSafe()) { BeginAbort("stalking became unsafe", frame.nowMs); return; }
            const auto player = api_.PlayerPed();
            const bool aimed = combatApi_.PlayerAimedPed(player) == actor_;
            const float distance = encounter_math::Distance2D(api_.EntityCoords(player), api_.EntityCoords(actor_));
            if (aimed || distance <= static_cast<float>(config_.encounter.confrontationDistance) ||
                frame.nowMs - stateStartedMs_ >= static_cast<std::uint64_t>(config_.encounter.stalkingMs)) {
                combatApi_.TaskStandStill(actor_, config_.encounter.confrontationMs + 200);
                presentationApi_.PlayShadowSmoke(api_.EntityCoords(actor_), 0.42F);
                Transition(SaintDenisState::Confrontation, frame.nowMs);
            }
            return;
        }
        case SaintDenisState::Confrontation:
            if (!ActorValid()) { BeginAbort("confrontation actor invalid", frame.nowMs); return; }
            if (!PreCombatStillSafe()) { BeginAbort("confrontation became unsafe", frame.nowMs); return; }
            if (frame.nowMs - stateStartedMs_ >= static_cast<std::uint64_t>(config_.encounter.confrontationMs)) {
                if (!registry_.SetCombatEnabled(actor_, BossOwner::Encounter, true)) {
                    BeginAbort("could not arm encounter combat ownership", frame.nowMs); return;
                }
                if (bossHud_.BeginBoss(actor_, config_.bossHud.displayName, frame.nowMs)) bossHud_.NotifyCombatActivity(frame.nowMs);
                logger_.Write(util::LogLevel::Info, "Saint Denis vampire encounter entered combat.");
                outsideSinceMs_ = 0;
                Transition(SaintDenisState::Combat, frame.nowMs);
            }
            return;
        case SaintDenisState::Combat: {
            if (!ActorValid(false)) { bossHud_.ForceHide(); BeginAbort("combat actor became invalid", frame.nowMs); return; }
            if (!api_.PedAlive(actor_)) {
                registry_.SetCombatEnabled(actor_, BossOwner::Encounter, false);
                bossHud_.EndBoss(true, frame.nowMs);
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
