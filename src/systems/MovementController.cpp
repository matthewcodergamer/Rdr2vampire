#include "nightwalker/systems/MovementController.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "nightwalker/systems/MovementMath.h"

namespace nightwalker::systems {
namespace {

float Distance2D(const game::Vec3& a, const game::Vec3& b) noexcept {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

MovementController::MovementController(
    game::IGameApi& api,
    game::IGameCombatApi& combatApi,
    game::IGameMovementApi& movementApi,
    game::IGamePresentationApi& presentationApi,
    DebugVampireSpawner& spawner,
    VampireAIController& vampireAi,
    util::Logger& logger,
    const core::Config& config) noexcept
    : api_(api),
      combatApi_(combatApi),
      movementApi_(movementApi),
      presentationApi_(presentationApi),
      spawner_(spawner),
      vampireAi_(vampireAi),
      logger_(logger),
      config_(config) {}

bool MovementController::Initialize() {
    movementWatchdog_.RestoreAll();
    ResetTransient();
    state_ = MovementState::Idle;
    return true;
}

void MovementController::Update(const core::FrameContext& frame) {
    const game::PedHandle currentOwned = spawner_.OwnedPed();
    if (ownedPed_ != 0 && currentOwned != ownedPed_) {
        RestoreMultiplier("owned vampire changed");
        ResetTransient();
    }
    ownedPed_ = currentOwned;

    if (!config_.debug.enabled || !config_.IsFeatureEnabled(core::Feature::Movement)) {
        RestoreMultiplier("movement feature disabled");
        state_ = MovementState::Idle;
        return;
    }

    if (!ValidOwnedVampire(ownedPed_)) {
        RestoreMultiplier("owned vampire invalid");
        ResetTransient();
        state_ = MovementState::Idle;
        return;
    }

    const game::PedHandle player = api_.PlayerPed();
    if (!api_.PedAlive(player)) {
        RestoreMultiplier("player invalid");
        state_ = MovementState::Restricted;
        return;
    }

    if (vampireAi_.State() != VampireAiState::Approach) {
        RestoreMultiplier("AI state is not continuous approach");
        state_ = MovementState::Idle;
        return;
    }

    if (IsMovementRestricted(ownedPed_, frame.nowMs)) {
        RestoreMultiplier("movement restriction active");
        state_ = MovementState::Restricted;
        return;
    }
    if (state_ == MovementState::Restricted) Transition(MovementState::Idle, frame.nowMs);

    if (!WantsBoost(ownedPed_, player)) {
        RestoreMultiplier("vampire is not in a chase-speed window");
        if (state_ != MovementState::Recovery) Transition(MovementState::Idle, frame.nowMs);
        return;
    }

    if (state_ == MovementState::Recovery) {
        if (frame.nowMs < recoveryUntilMs_) return;
        Transition(MovementState::Idle, frame.nowMs);
    }

    if (state_ == MovementState::Idle) {
        trailUnavailableLogged_ = false;
        nextTrailMs_ = frame.nowMs;
        Transition(MovementState::RampUp, frame.nowMs);
    }

    if (state_ == MovementState::RampUp) {
        const std::uint64_t elapsed = frame.nowMs >= stateStartedMs_
            ? frame.nowMs - stateStartedMs_
            : 0;
        const float multiplier = movement_math::RampMultiplier(
            elapsed,
            static_cast<std::uint32_t>(config_.movement.accelerationMs),
            static_cast<float>(config_.movement.sprintMoveRate));
        if (!ApplyMultiplier(ownedPed_, multiplier)) {
            RestoreMultiplier("move-rate native failed during ramp");
            Transition(MovementState::Restricted, frame.nowMs);
            return;
        }
        EmitTrail(ownedPed_, frame.nowMs);
        if (elapsed >= static_cast<std::uint64_t>(config_.movement.accelerationMs)) {
            Transition(MovementState::Boost, frame.nowMs);
        }
        return;
    }

    if (state_ == MovementState::Boost) {
        if (!ApplyMultiplier(ownedPed_, static_cast<float>(config_.movement.sprintMoveRate))) {
            RestoreMultiplier("move-rate native failed during boost");
            Transition(MovementState::Restricted, frame.nowMs);
            return;
        }
        EmitTrail(ownedPed_, frame.nowMs);
        if (frame.nowMs - stateStartedMs_ >=
            static_cast<std::uint64_t>(config_.movement.burstDurationMs)) {
            RestoreMultiplier("burst completed");
            recoveryUntilMs_ = frame.nowMs +
                static_cast<std::uint64_t>(config_.movement.recoveryMs);
            Transition(MovementState::Recovery, frame.nowMs);
        }
    }
}

bool MovementController::ValidOwnedVampire(game::PedHandle ped) const noexcept {
    return ped != 0 && ped == spawner_.OwnedPed() && api_.EntityExists(ped) &&
        api_.EntityModel(ped) == DebugVampireSpawner::kVampireModel && api_.PedAlive(ped);
}

bool MovementController::IsMovementRestricted(game::PedHandle ped, std::uint64_t nowMs) noexcept {
    const bool mounted = movementApi_.IsOnMount(ped);
    if (wasMounted_ && !mounted) {
        dismountRecoveryUntilMs_ = nowMs +
            static_cast<std::uint64_t>(config_.movement.dismountRecoveryMs);
    }
    wasMounted_ = mounted;

    if (mounted) return true;
    if (nowMs < dismountRecoveryUntilMs_) return true;
    if (movementApi_.IsSwimming(ped)) return true;
    if (movementApi_.IsFalling(ped)) return true;
    if (movementApi_.IsRagdoll(ped)) return true;
    return false;
}

bool MovementController::WantsBoost(game::PedHandle vampire, game::PedHandle player) const noexcept {
    const float distance = Distance2D(api_.EntityCoords(vampire), api_.EntityCoords(player));
    if (distance < static_cast<float>(config_.movement.activationDistance)) return false;
    const float speed = movement_math::HorizontalSpeed(combatApi_.EntityVelocity(vampire));
    return speed >= static_cast<float>(config_.movement.minVelocity);
}

bool MovementController::ApplyMultiplier(game::PedHandle ped, float multiplier) noexcept {
    const float safe = std::clamp(
        multiplier,
        1.0F,
        static_cast<float>(config_.movement.sprintMoveRate));

    if (!movementWatchdog_.IsOwned(core::OwnedState::Motion)) {
        const game::PedHandle captured = ped;
        const bool owned = movementWatchdog_.Own(core::OwnedState::Motion, [this, captured] {
            if (captured == 0 || !api_.EntityExists(captured) ||
                api_.EntityModel(captured) != DebugVampireSpawner::kVampireModel) {
                return;
            }
            if (!movementApi_.SetMoveRate(captured, 1.0F)) {
                logger_.Write(util::LogLevel::Error,
                    "Failed to restore owned vampire move-rate override to 1.0.");
            }
        });
        if (!owned) return false;
    }

    if (!movementApi_.SetMoveRate(ped, safe)) return false;
    lastAppliedMultiplier_ = safe;
    return true;
}

void MovementController::RestoreMultiplier(std::string_view reason) noexcept {
    if (!movementWatchdog_.IsOwned(core::OwnedState::Motion)) {
        lastAppliedMultiplier_ = 1.0F;
        return;
    }
    if (!movementWatchdog_.Restore(core::OwnedState::Motion)) {
        logger_.Write(util::LogLevel::Error,
            std::string("Movement cleanup failed while restoring move rate: ") + std::string(reason));
    } else if (config_.debug.enabled && lastAppliedMultiplier_ > 1.001F) {
        logger_.Write(util::LogLevel::Debug,
            std::string("Restored vampire move rate to normal: ") + std::string(reason));
    }
    lastAppliedMultiplier_ = 1.0F;
}

void MovementController::EmitTrail(game::PedHandle ped, std::uint64_t nowMs) noexcept {
    if (!config_.movement.trailFx || nowMs < nextTrailMs_) return;
    nextTrailMs_ = nowMs + static_cast<std::uint64_t>(config_.movement.trailIntervalMs);
    const bool played = presentationApi_.PlayShadowSmoke(api_.EntityCoords(ped), 0.28F);
    if (!played && config_.debug.enabled && !trailUnavailableLogged_) {
        trailUnavailableLogged_ = true;
        logger_.Write(util::LogLevel::Debug,
            "Movement trail FX is not ready/available; speed continues without it.");
    }
}

void MovementController::Transition(MovementState next, std::uint64_t nowMs) noexcept {
    if (config_.debug.enabled && state_ != next) {
        const std::uint64_t elapsed = stateStartedMs_ == 0 || nowMs < stateStartedMs_
            ? 0
            : nowMs - stateStartedMs_;
        logger_.Write(util::LogLevel::Debug,
            std::string("Movement state ") + StateName(state_) + " -> " + StateName(next) +
            " elapsedMs=" + std::to_string(elapsed));
    }
    state_ = next;
    stateStartedMs_ = nowMs;
}

void MovementController::ResetTransient() noexcept {
    ownedPed_ = 0;
    stateStartedMs_ = 0;
    recoveryUntilMs_ = 0;
    dismountRecoveryUntilMs_ = 0;
    nextTrailMs_ = 0;
    lastAppliedMultiplier_ = 1.0F;
    wasMounted_ = false;
    trailUnavailableLogged_ = false;
}

void MovementController::Cancel() noexcept {
    RestoreMultiplier("controller cancel");
    movementWatchdog_.RestoreAll();
    ResetTransient();
    state_ = MovementState::Idle;
}

void MovementController::Shutdown() noexcept {
    Cancel();
}

const char* MovementController::StateName(MovementState state) noexcept {
    switch (state) {
        case MovementState::Idle: return "Idle";
        case MovementState::RampUp: return "RampUp";
        case MovementState::Boost: return "Boost";
        case MovementState::Recovery: return "Recovery";
        case MovementState::Restricted: return "Restricted";
        default: return "Unknown";
    }
}

} // namespace nightwalker::systems
