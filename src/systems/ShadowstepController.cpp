#include "nightwalker/systems/ShadowstepController.h"

#include <algorithm>
#include <string>

#include "nightwalker/systems/ShadowstepMath.h"

namespace nightwalker::systems {
namespace {
constexpr float kMaximumPlayerDriftBeforeRelocate = 1.25F;
constexpr float kRelocateVerificationTolerance = 1.0F;
constexpr std::uint32_t kStressTarget = 100;
}

ShadowstepController::ShadowstepController(
    game::IGameApi& api,
    util::Logger& logger,
    const core::Config& config) noexcept
    : api_(api),
      logger_(logger),
      config_(config),
      resolver_(api, config.shadowstep),
      carryResolverSettings_(config.shadowstep),
      carryResolver_(api, carryResolverSettings_) {}

bool ShadowstepController::Initialize() {
    state_ = ShadowstepState::Idle;
    stateStartedMs_ = 0;
    cooldownUntilMs_ = 0;
    hiddenUntilMs_ = 0;
    meleeBufferUntilMs_ = 0;
    stressSuccessCount_ = 0;
    meleeBuffered_ = false;
    fxUnavailableLogged_ = false;
    ClearTransient();
    presentationWatchdog_.RestoreAll();
    if (presentationSettings_.smokeFx) presentationApi_.RequestShadowSmoke();
    return true;
}

void ShadowstepController::RequestForward(std::uint64_t nowMs) noexcept {
    if (!config_.debug.enabled || !config_.IsFeatureEnabled(core::Feature::Shadowstep)) return;
    if (state_ != ShadowstepState::Idle) {
        logger_.Write(util::LogLevel::Debug,
            std::string("Shadowstep request ignored while state=") + StateName(state_));
        return;
    }
    if (nowMs < cooldownUntilMs_) {
        logger_.Write(util::LogLevel::Debug,
            std::string("Shadowstep request ignored during cooldown; remainingMs=") +
            std::to_string(cooldownUntilMs_ - nowMs));
        return;
    }
    Transition(ShadowstepState::ResolveIntent, nowMs);
}

void ShadowstepController::Update(const core::FrameContext& frame) {
    if (!config_.IsFeatureEnabled(core::Feature::Shadowstep)) {
        if (state_ != ShadowstepState::Idle) Cancel();
        return;
    }
    if (StateTimedOut(frame.nowMs)) {
        Fail("state watchdog timeout", frame.nowMs);
        return;
    }

    switch (state_) {
        case ShadowstepState::Idle:
            return;

        case ShadowstepState::ResolveIntent: {
            player_ = api_.PlayerPed();
            if (!api_.PedAlive(player_)) {
                Fail("player is not valid/alive", frame.nowMs);
                return;
            }
            startPosition_ = api_.EntityCoords(player_);
            forwardDirection_ = api_.EntityForward(player_);
            game::Vec3 normalized{};
            if (!shadowstep_math::NormalizeHorizontal(forwardDirection_, normalized)) {
                Fail("player forward vector is invalid", frame.nowMs);
                return;
            }
            forwardDirection_ = normalized;
            const game::Vec3 requested = shadowstep_math::AddScaled(
                startPosition_, forwardDirection_, static_cast<float>(config_.shadowstep.quickDistance));
            logger_.Write(util::LogLevel::Debug,
                "Shadowstep requested from (" + std::to_string(startPosition_.x) + "," +
                std::to_string(startPosition_.y) + "," + std::to_string(startPosition_.z) +
                ") to (" + std::to_string(requested.x) + "," + std::to_string(requested.y) +
                "," + std::to_string(requested.z) + ").");
            Transition(ShadowstepState::ValidateDestination, frame.nowMs);
            return;
        }

        case ShadowstepState::ValidateDestination: {
            if (ValidationTimedOut(frame.nowMs)) {
                Fail("destination validation timeout", frame.nowMs);
                return;
            }
            resolution_ = resolver_.Resolve(player_, startPosition_, forwardDirection_);
            if (!resolution_.valid) {
                stressSuccessCount_ = 0;
                logger_.Write(util::LogLevel::Debug,
                    std::string("Shadowstep rejected: ") + ShadowstepResolver::ReasonText(resolution_.reason));
                Transition(ShadowstepState::Error, frame.nowMs);
                return;
            }

            carryResolution_ = {};
            if (presentationSettings_.carryMeters > 0.05F) {
                carryResolution_ = carryResolver_.Resolve(
                    player_, resolution_.finalPosition, forwardDirection_);
                if (!carryResolution_.valid) {
                    logger_.Write(util::LogLevel::Debug,
                        std::string("Arrival carry disabled for this step: ") +
                        ShadowstepResolver::ReasonText(carryResolution_.reason));
                }
            }

            logger_.Write(util::LogLevel::Debug,
                "Shadowstep resolved to (" + std::to_string(resolution_.finalPosition.x) + "," +
                std::to_string(resolution_.finalPosition.y) + "," +
                std::to_string(resolution_.finalPosition.z) + ") distance=" +
                std::to_string(resolution_.finalDistance) +
                (resolution_.shortened ? " shortened=true" : " shortened=false"));
            Transition(ShadowstepState::Depart, frame.nowMs);
            return;
        }

        case ShadowstepState::Depart:
            SampleMelee(frame.nowMs);
            if (presentationSettings_.smokeFx &&
                !presentationApi_.PlayShadowSmoke(startPosition_, 0.82F) &&
                !fxUnavailableLogged_) {
                fxUnavailableLogged_ = true;
                logger_.Write(util::LogLevel::Debug,
                    "Shadowstep smoke unavailable at departure; continuing without blocking cleanup.");
            }
            if (!BeginHiddenTransit(frame.nowMs)) {
                Fail("could not enter hidden transit safely", frame.nowMs);
                return;
            }
            Transition(ShadowstepState::Relocate, frame.nowMs);
            return;

        case ShadowstepState::Relocate: {
            SampleMelee(frame.nowMs);
            if (!api_.PedAlive(player_)) {
                Fail("player became invalid before relocation", frame.nowMs);
                return;
            }
            const game::Vec3 current = api_.EntityCoords(player_);
            if (shadowstep_math::Distance3D(current, startPosition_) > kMaximumPlayerDriftBeforeRelocate) {
                Fail("player moved too far after destination validation", frame.nowMs);
                return;
            }
            if (!api_.SetEntityCoordsNoOffset(player_, resolution_.finalPosition)) {
                Fail("native relocation failed", frame.nowMs);
                return;
            }
            const game::Vec3 actual = api_.EntityCoords(player_);
            if (shadowstep_math::Distance3D(actual, resolution_.finalPosition) > kRelocateVerificationTolerance) {
                bool rolledBack = false;
                if (api_.PedAlive(player_) && api_.SetEntityCoordsNoOffset(player_, startPosition_)) {
                    const game::Vec3 rollbackPosition = api_.EntityCoords(player_);
                    rolledBack = shadowstep_math::Distance3D(rollbackPosition, startPosition_) <=
                                 kRelocateVerificationTolerance;
                }
                logger_.Write(rolledBack ? util::LogLevel::Warning : util::LogLevel::Error,
                    rolledBack
                        ? "Shadowstep relocation verification failed; player rolled back to start."
                        : "Shadowstep relocation verification failed and rollback was not confirmed.");
                Fail("relocation verification mismatch", frame.nowMs);
                return;
            }
            Transition(ShadowstepState::HiddenTransit, frame.nowMs);
            return;
        }

        case ShadowstepState::HiddenTransit:
            SampleMelee(frame.nowMs);
            if (frame.nowMs < hiddenUntilMs_) return;
            RestorePresentation();
            if (presentationSettings_.smokeFx &&
                !presentationApi_.PlayShadowSmoke(resolution_.finalPosition, 1.05F) &&
                !fxUnavailableLogged_) {
                fxUnavailableLogged_ = true;
                logger_.Write(util::LogLevel::Debug,
                    "Shadowstep smoke unavailable at arrival; presentation continues safely.");
            }
            Transition(ShadowstepState::Arrive, frame.nowMs);
            return;

        case ShadowstepState::Arrive:
            ++stressSuccessCount_;
            logger_.Write(util::LogLevel::Debug,
                "Shadowstep success; consecutiveStressCounter=" + std::to_string(stressSuccessCount_) +
                "/" + std::to_string(kStressTarget));
            if (stressSuccessCount_ == kStressTarget) {
                logger_.Write(util::LogLevel::Info,
                    "Shadowstep stress milestone reached: 100 consecutive successful relocations.");
            }
            carryStart_ = api_.EntityCoords(player_);
            Transition(carryResolution_.valid ? ShadowstepState::ArrivalCarry
                                              : ShadowstepState::MeleeWindow,
                       frame.nowMs);
            return;

        case ShadowstepState::ArrivalCarry:
            SampleMelee(frame.nowMs);
            if (UpdateArrivalCarry(frame.nowMs)) {
                Transition(ShadowstepState::MeleeWindow, frame.nowMs);
            }
            return;

        case ShadowstepState::MeleeWindow:
            if (meleeBuffered_) {
                if (presentationApi_.MeleeInputPressed()) {
                    logger_.Write(util::LogLevel::Debug,
                        "Buffered melee remains physically held; normal RDR2 input stays live.");
                } else if (frame.nowMs <= meleeBufferUntilMs_ && presentationApi_.PulseMeleeInput()) {
                    logger_.Write(util::LogLevel::Debug, "Buffered melee input handed back to RDR2.");
                } else {
                    logger_.Write(util::LogLevel::Debug,
                        "Buffered melee tap expired without synthetic replay; normal controls remain live.");
                }
                meleeBuffered_ = false;
            }
            Transition(ShadowstepState::Recovery, frame.nowMs);
            return;

        case ShadowstepState::Recovery:
            cooldownUntilMs_ = frame.nowMs + static_cast<std::uint64_t>(config_.shadowstep.cooldownMs);
            Transition(ShadowstepState::Cooldown, frame.nowMs);
            return;

        case ShadowstepState::Cooldown:
            if (frame.nowMs >= cooldownUntilMs_) {
                ClearTransient();
                Transition(ShadowstepState::Idle, frame.nowMs);
            }
            return;

        case ShadowstepState::Error:
            RestorePresentation();
            ClearTransient();
            Transition(ShadowstepState::Idle, frame.nowMs);
            return;
    }
}

void ShadowstepController::Cancel() noexcept {
    if (state_ != ShadowstepState::Idle) {
        logger_.Write(util::LogLevel::Debug,
            std::string("Shadowstep cancelled from state=") + StateName(state_));
    }
    stressSuccessCount_ = 0;
    RestorePresentation();
    ClearTransient();
    state_ = ShadowstepState::Idle;
    stateStartedMs_ = 0;
    cooldownUntilMs_ = 0;
}

void ShadowstepController::Shutdown() noexcept {
    Cancel();
    presentationApi_.ReleaseShadowSmoke();
}

void ShadowstepController::Transition(ShadowstepState next, std::uint64_t nowMs) noexcept {
    if (config_.debug.enabled && state_ != next) {
        const std::uint64_t elapsed = stateStartedMs_ == 0 ? 0 : nowMs - stateStartedMs_;
        logger_.Write(util::LogLevel::Debug,
            std::string("Shadowstep state ") + StateName(state_) + " -> " + StateName(next) +
            " elapsedMs=" + std::to_string(elapsed));
    }
    state_ = next;
    stateStartedMs_ = nowMs;
}

void ShadowstepController::Fail(const char* reason, std::uint64_t nowMs) noexcept {
    stressSuccessCount_ = 0;
    RestorePresentation();
    logger_.Write(util::LogLevel::Warning,
        std::string("Shadowstep aborted safely: ") + reason + " state=" + StateName(state_));
    Transition(ShadowstepState::Error, nowMs);
}

void ShadowstepController::ClearTransient() noexcept {
    player_ = 0;
    startPosition_ = {};
    forwardDirection_ = {};
    carryStart_ = {};
    resolution_ = {};
    carryResolution_ = {};
    hiddenUntilMs_ = 0;
    meleeBufferUntilMs_ = 0;
    meleeBuffered_ = false;
}

bool ShadowstepController::ValidationTimedOut(std::uint64_t nowMs) const noexcept {
    if (stateStartedMs_ == 0) return false;
    return nowMs - stateStartedMs_ >
        static_cast<std::uint64_t>(config_.shadowstep.validationTimeoutMs);
}

const char* ShadowstepController::StateName(ShadowstepState state) noexcept {
    switch (state) {
        case ShadowstepState::Idle: return "Idle";
        case ShadowstepState::ResolveIntent: return "ResolveIntent";
        case ShadowstepState::ValidateDestination: return "ValidateDestination";
        case ShadowstepState::Depart: return "Depart";
        case ShadowstepState::Relocate: return "Relocate";
        case ShadowstepState::HiddenTransit: return "HiddenTransit";
        case ShadowstepState::Arrive: return "Arrive";
        case ShadowstepState::ArrivalCarry: return "ArrivalCarry";
        case ShadowstepState::MeleeWindow: return "MeleeWindow";
        case ShadowstepState::Recovery: return "Recovery";
        case ShadowstepState::Cooldown: return "Cooldown";
        case ShadowstepState::Error: return "Error";
        default: return "Unknown";
    }
}

} // namespace nightwalker::systems
