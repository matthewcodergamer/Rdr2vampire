#include "nightwalker/systems/SaintDenisDirector.h"

namespace nightwalker::systems {

void SaintDenisDirector::UpdateSetup(const core::FrameContext& frame) {
    switch (state_) {
        case SaintDenisState::Dormant:
            if (frame.nowMs < nextEligibilityCheckMs_) return;
            nextEligibilityCheckMs_ = frame.nowMs +
                static_cast<std::uint64_t>(config_.encounter.eligibilityPollMs);
            if (EligibleNow()) Transition(SaintDenisState::Eligible, frame.nowMs);
            return;

        case SaintDenisState::Eligible:
            if (!EligibleNow()) {
                Transition(SaintDenisState::Dormant, frame.nowMs);
                return;
            }
            nextOmenMs_ = frame.nowMs;
            Transition(SaintDenisState::Omen, frame.nowMs);
            return;

        case SaintDenisState::Omen: {
            if (!PreCombatStillSafe()) {
                BeginAbort("eligibility changed during omen", frame.nowMs);
                return;
            }
            if (frame.nowMs >= nextOmenMs_) {
                const game::Vec3 omen{
                    static_cast<float>(config_.encounter.centerX),
                    static_cast<float>(config_.encounter.centerY),
                    static_cast<float>(config_.encounter.centerZ + 0.35)};
                presentationApi_.PlayShadowSmoke(omen, 0.34F);
                nextOmenMs_ = frame.nowMs +
                    static_cast<std::uint64_t>(config_.encounter.omenPulseMs);
            }
            if (frame.nowMs - stateStartedMs_ >=
                static_cast<std::uint64_t>(config_.encounter.omenDurationMs)) {
                if (!BeginModelRequest(frame.nowMs)) {
                    BeginAbort("model request failed", frame.nowMs);
                    return;
                }
                nextSpawnRetryMs_ = frame.nowMs;
                Transition(SaintDenisState::SpawnPending, frame.nowMs);
            }
            return;
        }

        case SaintDenisState::SpawnPending: {
            if (!PreCombatStillSafe()) {
                BeginAbort("eligibility changed while spawning", frame.nowMs);
                return;
            }
            const auto status = modelRequest_.Update(api_, frame.nowMs);
            if (status == game::ModelStreamStatus::TimedOut ||
                status == game::ModelStreamStatus::InvalidModel) {
                BeginAbort("cs_vampire model load failed", frame.nowMs);
                return;
            }
            if (frame.nowMs - stateStartedMs_ >
                static_cast<std::uint64_t>(config_.encounter.spawnTimeoutMs)) {
                BeginAbort("no safe encounter spawn point became available", frame.nowMs);
                return;
            }
            if (status == game::ModelStreamStatus::Loaded && frame.nowMs >= nextSpawnRetryMs_) {
                if (!SpawnActor(frame.nowMs)) {
                    nextSpawnRetryMs_ = frame.nowMs +
                        static_cast<std::uint64_t>(config_.encounter.spawnRetryMs);
                }
            }
            return;
        }

        default:
            return;
    }
}

} // namespace nightwalker::systems
