#pragma once

#include <cmath>
#include <cstdint>

#include "nightwalker/game/GameApi.h"

namespace nightwalker::core {

enum class RecoveryTrigger {
    None,
    UnsafeStoryState,
    LongFrameGap,
    ClockDiscontinuity,
    PlayerHandleChanged,
    WorldDiscontinuity,
};

struct RuntimeObservation final {
    std::uint64_t nowMs{0};
    bool storySafe{false};
    game::PedHandle playerPed{0};
    bool positionKnown{false};
    game::Vec3 playerPosition{};
};

struct RecoveryDecision final {
    RecoveryTrigger trigger{RecoveryTrigger::None};
    bool cleanupRequested{false};
    bool suspendUpdates{false};
};

// Pure long-session safety policy. This deliberately owns no RDR2 state; Runtime
// supplies observations and performs cleanup through the existing controllers.
class LongSessionGuard final {
public:
    static constexpr std::uint64_t kResumeDelayMs = 750;
    static constexpr std::uint64_t kLongFrameGapMs = 2500;
    static constexpr float kWorldDiscontinuityMeters = 120.0F;

    [[nodiscard]] RecoveryDecision Observe(const RuntimeObservation& observation) noexcept {
        if (!observation.storySafe) {
            const bool firstUnsafeTick = !unsafeObserved_;
            unsafeObserved_ = true;
            quarantine_ = true;
            resumeAtMs_ = 0;
            baselineValid_ = false;
            lastObservedMs_ = observation.nowMs;
            return {
                firstUnsafeTick ? RecoveryTrigger::UnsafeStoryState : RecoveryTrigger::None,
                firstUnsafeTick,
                true,
            };
        }

        if (unsafeObserved_) {
            if (resumeAtMs_ == 0) resumeAtMs_ = observation.nowMs + kResumeDelayMs;
            if (observation.nowMs < resumeAtMs_) return {RecoveryTrigger::None, false, true};
            unsafeObserved_ = false;
            quarantine_ = false;
            resumeAtMs_ = 0;
            Seed(observation);
            return {};
        }

        if (quarantine_) {
            if (observation.nowMs < resumeAtMs_) return {RecoveryTrigger::None, false, true};
            quarantine_ = false;
            resumeAtMs_ = 0;
            Seed(observation);
            return {};
        }

        if (!baselineValid_) {
            Seed(observation);
            return {};
        }

        if (observation.nowMs < lastObservedMs_) {
            return BeginQuarantine(RecoveryTrigger::ClockDiscontinuity, observation.nowMs);
        }
        if (observation.nowMs - lastObservedMs_ > kLongFrameGapMs) {
            return BeginQuarantine(RecoveryTrigger::LongFrameGap, observation.nowMs);
        }
        if (observation.playerPed != lastPlayerPed_) {
            return BeginQuarantine(RecoveryTrigger::PlayerHandleChanged, observation.nowMs);
        }
        if (observation.positionKnown && lastPositionKnown_ &&
            PositionValid(observation.playerPosition) && PositionValid(lastPosition_)) {
            const double dx = static_cast<double>(observation.playerPosition.x) - lastPosition_.x;
            const double dy = static_cast<double>(observation.playerPosition.y) - lastPosition_.y;
            const double dz = static_cast<double>(observation.playerPosition.z) - lastPosition_.z;
            const double distanceSquared = dx * dx + dy * dy + dz * dz;
            const double threshold = static_cast<double>(kWorldDiscontinuityMeters);
            if (distanceSquared > threshold * threshold) {
                return BeginQuarantine(RecoveryTrigger::WorldDiscontinuity, observation.nowMs);
            }
        }

        Seed(observation);
        return {};
    }

    void Reset() noexcept {
        baselineValid_ = false;
        unsafeObserved_ = false;
        quarantine_ = false;
        lastObservedMs_ = 0;
        resumeAtMs_ = 0;
        lastPlayerPed_ = 0;
        lastPositionKnown_ = false;
        lastPosition_ = {};
    }

    [[nodiscard]] static const char* TriggerName(RecoveryTrigger trigger) noexcept {
        switch (trigger) {
            case RecoveryTrigger::UnsafeStoryState: return "unsafe Story Mode transition";
            case RecoveryTrigger::LongFrameGap: return "long script/frame gap";
            case RecoveryTrigger::ClockDiscontinuity: return "runtime clock discontinuity";
            case RecoveryTrigger::PlayerHandleChanged: return "player handle changed";
            case RecoveryTrigger::WorldDiscontinuity: return "large world-position discontinuity";
            case RecoveryTrigger::None:
            default: return "none";
        }
    }

private:
    [[nodiscard]] static bool PositionValid(const game::Vec3& position) noexcept {
        return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z);
    }

    void Seed(const RuntimeObservation& observation) noexcept {
        baselineValid_ = true;
        lastObservedMs_ = observation.nowMs;
        lastPlayerPed_ = observation.playerPed;
        lastPositionKnown_ = observation.positionKnown && PositionValid(observation.playerPosition);
        lastPosition_ = lastPositionKnown_ ? observation.playerPosition : game::Vec3{};
    }

    [[nodiscard]] RecoveryDecision BeginQuarantine(
        RecoveryTrigger trigger, std::uint64_t nowMs) noexcept {
        quarantine_ = true;
        resumeAtMs_ = nowMs + kResumeDelayMs;
        baselineValid_ = false;
        return {trigger, true, true};
    }

    bool baselineValid_{false};
    bool unsafeObserved_{false};
    bool quarantine_{false};
    std::uint64_t lastObservedMs_{0};
    std::uint64_t resumeAtMs_{0};
    game::PedHandle lastPlayerPed_{0};
    bool lastPositionKnown_{false};
    game::Vec3 lastPosition_{};
};

} // namespace nightwalker::core
