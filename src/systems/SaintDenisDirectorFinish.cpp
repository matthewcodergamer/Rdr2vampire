#include "nightwalker/systems/SaintDenisDirector.h"

#include "nightwalker/systems/EncounterMath.h"

namespace nightwalker::systems {

void SaintDenisDirector::UpdateFinish(const core::FrameContext& frame) {
    switch (state_) {
        case SaintDenisState::Resolution:
            if (frame.nowMs - stateStartedMs_ >=
                static_cast<std::uint64_t>(config_.encounter.resolutionHoldMs)) {
                cleanupResolved_ = true;
                Transition(SaintDenisState::Cleanup, frame.nowMs);
            }
            return;

        case SaintDenisState::Abort:
            cleanupResolved_ = false;
            Transition(SaintDenisState::Cleanup, frame.nowMs);
            return;

        case SaintDenisState::Cleanup:
            if (!CleanupActor(cleanupResolved_)) return;
            cooldownUntilGameSeconds_ = cleanupResolved_
                ? encounter_math::AddCooldownHours(
                    encounterApi_.GameSecondsSinceBaseYear(), config_.encounter.respawnCooldownHours)
                : encounter_math::AddCooldownMinutes(
                    encounterApi_.GameSecondsSinceBaseYear(), config_.encounter.abortCooldownMinutes);
            cleanupResolved_ = false;
            Transition(SaintDenisState::Cooldown, frame.nowMs);
            return;

        case SaintDenisState::Cooldown:
            if (encounter_math::CooldownExpired(
                    encounterApi_.GameSecondsSinceBaseYear(), cooldownUntilGameSeconds_)) {
                nextEligibilityCheckMs_ = frame.nowMs;
                Transition(SaintDenisState::Dormant, frame.nowMs);
            }
            return;

        default:
            return;
    }
}

} // namespace nightwalker::systems
