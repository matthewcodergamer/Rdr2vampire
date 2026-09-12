#include "nightwalker/systems/VampireCombatController.h"

#include <algorithm>
#include <string>

#include "nightwalker/systems/MotionImpulseMath.h"

namespace nightwalker::systems {
namespace {
constexpr std::uint64_t kReleaseSettleMs = 180;
constexpr float kTraceLift = 0.65F;
}

VampireCombatController::VampireCombatController(
    game::IGameApi& api,
    game::IGameCombatApi& combatApi,
    game::IGameFeedingApi& feedingApi,
    game::IGameMovementApi& movementApi,
    game::IGamePhysicalApi& physicalApi,
    DebugVampireSpawner& spawner,
    FeedingController& feedingController,
    util::Logger& logger,
    core::Config& config) noexcept
    : api_(api), combatApi_(combatApi), feedingApi_(feedingApi), movementApi_(movementApi),
      physicalApi_(physicalApi), spawner_(spawner), feedingController_(feedingController),
      logger_(logger), config_(config) {}

bool VampireCombatController::Initialize() {
    Reset();
    logger_.Write(util::LogLevel::Info,
        "VampireCombatController initialized; physical moves expose no custom ability HUD.");
    return true;
}

bool VampireCombatController::RequestPlayerDebug(CombatMove move, std::uint64_t nowMs) noexcept {
    if (!config_.debug.enabled || IsActive()) return false;
    const game::PedHandle actor = api_.PlayerPed();
    const game::PedHandle target = combatApi_.PlayerAimedPed(actor);
    return BeginRequest(move, CombatRole::PlayerDebug, actor, target, nowMs, false);
}

bool VampireCombatController::RequestBoss(
    CombatMove move,
    game::PedHandle actor,
    game::PedHandle target,
    std::uint64_t nowMs,
    bool preTelegraphed) noexcept {
    if (IsActive()) return false;
    return BeginRequest(move, CombatRole::Boss, actor, target, nowMs, preTelegraphed);
}

bool VampireCombatController::RequestGrabFollowup(CombatMove move) noexcept {
    if (role_ != CombatRole::PlayerDebug || state_ != CombatState::Hold || move_ != CombatMove::GrabControl) return false;
    if (move != CombatMove::GrabThrow && move != CombatMove::CombatFeed) return false;
    move_ = move;
    if (config_.debug.enabled) {
        logger_.Write(util::LogLevel::Debug,
            std::string("Owned grab follow-up -> ") + MoveName(move_));
    }
    return true;
}

void VampireCombatController::CancelForActor(game::PedHandle actor) noexcept {
    if (IsActiveFor(actor)) Abort("actor cancelled");
}

bool VampireCombatController::BeginRequest(
    CombatMove move,
    CombatRole role,
    game::PedHandle actor,
    game::PedHandle target,
    std::uint64_t nowMs,
    bool preTelegraphed) noexcept {
    if (!config_.IsFeatureEnabled(core::Feature::Combat) || IsActive()) return false;
    move_ = move;
    role_ = role;
    actor_ = actor;
    target_ = target;
    preTelegraphed_ = preTelegraphed;
    strikeBonusApplied_ = false;
    feedResultApplied_ = false;
    actorTaskOwned_ = false;
    targetTaskOwned_ = false;

    const bool directFeed = move_ == CombatMove::CombatFeed && role_ == CombatRole::PlayerDebug;
    if (!ValidateInitial(directFeed)) {
        Reset();
        return false;
    }

    const int windup = WindupMs();
    if (windup > 0) {
        feedingApi_.FacePedToward(actor_, target_);
        combatApi_.TaskStandStill(actor_, windup);
        actorTaskOwned_ = true;
    }
    Enter(CombatState::Telegraph, nowMs);
    if (config_.debug.enabled) {
        logger_.Write(util::LogLevel::Debug,
            std::string("Combat request accepted move=") + MoveName(move_) +
            " actor=" + std::to_string(actor_) + " target=" + std::to_string(target_));
    }
    return true;
}

bool VampireCombatController::ValidateInitial(bool directFeed) const noexcept {
    if (actor_ == 0 || target_ == 0 || actor_ == target_) return false;
    if (!api_.PedAlive(actor_) || !api_.PedAlive(target_)) return false;
    if (!motion_impulse_math::WithinRange(
            api_.EntityCoords(actor_), api_.EntityCoords(target_), config_.combat.maxDistance)) return false;
    if (!feedingApi_.HasClearLos(actor_, target_)) return false;
    if (feedingApi_.IsPedRestricted(actor_)) return false;

    if (role_ == CombatRole::Boss) {
        if (actor_ != spawner_.OwnedPed() ||
            api_.EntityModel(actor_) != DebugVampireSpawner::kVampireModel) return false;
    } else {
        if (actor_ != api_.PlayerPed()) return false;
        if (feedingApi_.IsMissionEntity(target_)) return false;
    }

    const bool needsHuman = move_ == CombatMove::GrabControl || move_ == CombatMove::GrabThrow ||
        move_ == CombatMove::CombatFeed;
    if (needsHuman && !feedingApi_.IsHuman(target_)) return false;

    if (directFeed) return movementApi_.IsRagdoll(target_);
    if (feedingApi_.IsPedRestricted(target_)) return false;
    return true;
}

bool VampireCombatController::ValidateLive() const noexcept {
    if (actor_ == 0 || target_ == 0 || !api_.PedAlive(actor_)) return false;
    if (role_ == CombatRole::Boss &&
        (actor_ != spawner_.OwnedPed() || api_.EntityModel(actor_) != DebugVampireSpawner::kVampireModel)) return false;
    if (!api_.EntityExists(target_)) return false;
    if (!motion_impulse_math::WithinRange(
            api_.EntityCoords(actor_), api_.EntityCoords(target_), config_.combat.maxDistance + 0.85)) return false;
    return feedingApi_.HasClearLos(actor_, target_);
}

void VampireCombatController::Update(const core::FrameContext& frame) {
    if (state_ == CombatState::Idle) return;
    if (!config_.IsFeatureEnabled(core::Feature::Combat)) { Abort("combat disabled"); return; }
    if (TimedOut(frame.nowMs)) { Abort("state timeout"); return; }
    if (!api_.PedAlive(actor_)) { Abort("actor invalid/dead"); return; }

    if (!api_.PedAlive(target_)) {
        if (state_ == CombatState::Strike || state_ == CombatState::Feed) {
            CleanupOwnedState();
            Enter(CombatState::Recover, frame.nowMs);
            return;
        }
        Abort("target invalid/dead");
        return;
    }

    if (state_ != CombatState::Release && !ValidateLive()) {
        Abort("participants separated or obstructed");
        return;
    }

    switch (state_) {
        case CombatState::Telegraph:
            if (Elapsed(frame.nowMs) < static_cast<std::uint64_t>(WindupMs())) return;
            if (move_ == CombatMove::ShadowstepStrike || move_ == CombatMove::HeavyStrike) {
                BeginStrike(frame.nowMs);
            } else if (move_ == CombatMove::CombatFeed && role_ == CombatRole::PlayerDebug &&
                       movementApi_.IsRagdoll(target_)) {
                BeginFeed(frame.nowMs, false);
            } else {
                BeginAlignment(frame.nowMs);
            }
            return;

        case CombatState::Align:
            if (Elapsed(frame.nowMs) >= static_cast<std::uint64_t>(config_.combat.grabAlignMs)) {
                BeginHold(frame.nowMs);
            }
            return;

        case CombatState::Hold:
            if (Elapsed(frame.nowMs) < static_cast<std::uint64_t>(config_.combat.grabHoldMs)) return;
            if (move_ == CombatMove::GrabThrow) BeginRelease(frame.nowMs);
            else if (move_ == CombatMove::CombatFeed) BeginFeed(frame.nowMs, true);
            else {
                CleanupOwnedState();
                Enter(CombatState::Recover, frame.nowMs);
            }
            return;

        case CombatState::Strike:
            if (role_ == CombatRole::Boss && watchdog_.IsOwned(core::OwnedState::Motion)) {
                movementApi_.SetMoveRate(actor_, static_cast<float>(config_.combat.strikeMoveRate));
            }
            ApplyStrikeBonus();
            if (Elapsed(frame.nowMs) >= static_cast<std::uint64_t>(config_.combat.strikeWindowMs)) {
                CleanupOwnedState();
                Enter(CombatState::Recover, frame.nowMs);
            }
            return;

        case CombatState::Release:
            if (Elapsed(frame.nowMs) >= kReleaseSettleMs) {
                CleanupOwnedState();
                Enter(CombatState::Recover, frame.nowMs);
            }
            return;

        case CombatState::Feed:
            if (Elapsed(frame.nowMs) >= static_cast<std::uint64_t>(config_.combat.feedHoldMs)) {
                ApplyFeedResult();
                CleanupOwnedState();
                Enter(CombatState::Recover, frame.nowMs);
            }
            return;

        case CombatState::Recover:
            if (Elapsed(frame.nowMs) >= static_cast<std::uint64_t>(config_.combat.recoveryMs)) Reset();
            return;

        case CombatState::Abort:
            Reset();
            return;

        case CombatState::Idle:
            return;
    }
}

void VampireCombatController::BeginStrike(std::uint64_t nowMs) noexcept {
    physicalApi_.ClearContactSource(target_);
    combatApi_.TaskCombatPed(actor_, target_);
    actorTaskOwned_ = true;

    if (role_ == CombatRole::Boss && config_.combat.strikeMoveRate > 1.001) {
        const game::PedHandle actor = actor_;
        if (watchdog_.Own(core::OwnedState::Motion, [this, actor] {
                if (actor != 0 && api_.EntityExists(actor)) movementApi_.SetMoveRate(actor, 1.0F);
            })) {
            movementApi_.SetMoveRate(actor_, static_cast<float>(config_.combat.strikeMoveRate));
        }
    }
    Enter(CombatState::Strike, nowMs);
}

void VampireCombatController::BeginAlignment(std::uint64_t nowMs) noexcept {
    actorTaskOwned_ = feedingApi_.FacePedToward(actor_, target_) || actorTaskOwned_;
    targetTaskOwned_ = feedingApi_.FacePedToward(target_, actor_) || targetTaskOwned_;
    Enter(CombatState::Align, nowMs);
}

void VampireCombatController::BeginHold(std::uint64_t nowMs) noexcept {
    if (feedingApi_.StartGrapple(actor_, target_)) {
        actorTaskOwned_ = true;
        targetTaskOwned_ = true;
    } else {
        feedingApi_.StandStill(actor_, config_.combat.grabHoldMs);
        feedingApi_.StandStill(target_, config_.combat.grabHoldMs);
        actorTaskOwned_ = true;
        targetTaskOwned_ = true;
        logger_.Write(util::LogLevel::Warning,
            "Short grapple did not start; using controlled hold fallback.");
    }
    Enter(CombatState::Hold, nowMs);
}

void VampireCombatController::BeginRelease(std::uint64_t nowMs) noexcept {
    if (actorTaskOwned_ && api_.EntityExists(actor_)) feedingApi_.ClearTasks(actor_);
    if (targetTaskOwned_ && api_.EntityExists(target_)) feedingApi_.ClearTasks(target_);
    actorTaskOwned_ = false;
    targetTaskOwned_ = false;

    if (feedingApi_.IsPedRestricted(target_)) { Abort("target cannot safely enter physical release"); return; }

    const auto plan = motion_impulse_math::BuildPlan(
        api_.EntityCoords(actor_), api_.EntityCoords(target_),
        config_.combat.throwHorizontalForce, config_.combat.throwUpForce,
        config_.combat.throwProjectionMeters);
    if (!plan.valid) { Abort("release direction invalid"); return; }

    game::Vec3 traceStart = api_.EntityCoords(target_);
    traceStart.z += kTraceLift;
    game::Vec3 traceEnd = plan.projectedEnd;
    traceEnd.z += kTraceLift;
    const auto trace = api_.RaycastWorld(traceStart, traceEnd, target_);
    const bool clearPath = trace.conclusive && !trace.hit;

    if (!physicalApi_.SetRagdoll(target_, config_.combat.throwRagdollMs)) {
        Abort("ragdoll request failed");
        return;
    }
    if (clearPath) {
        physicalApi_.ApplyImpulse(target_, plan.impulse);
    } else if (config_.debug.enabled) {
        logger_.Write(util::LogLevel::Debug,
            trace.conclusive ? "Release impulse suppressed by obstruction." :
                               "Release impulse suppressed because trace was inconclusive.");
    }
    Enter(CombatState::Release, nowMs);
}

void VampireCombatController::BeginFeed(std::uint64_t nowMs, bool fromOwnedHold) noexcept {
    if (fromOwnedHold) {
        if (actorTaskOwned_ && api_.EntityExists(actor_)) feedingApi_.ClearTasks(actor_);
        if (targetTaskOwned_ && api_.EntityExists(target_)) feedingApi_.ClearTasks(target_);
        actorTaskOwned_ = false;
        targetTaskOwned_ = false;
    }

    feedingApi_.FacePedToward(actor_, target_);
    feedingApi_.StandStill(actor_, config_.combat.feedHoldMs);
    actorTaskOwned_ = true;
    if (!movementApi_.IsRagdoll(target_)) {
        feedingApi_.StandStill(target_, config_.combat.feedHoldMs);
        targetTaskOwned_ = true;
    }
    Enter(CombatState::Feed, nowMs);
}

void VampireCombatController::ApplyStrikeBonus() noexcept {
    if (strikeBonusApplied_ || config_.combat.strikeBonus <= 0) return;
    if (!physicalApi_.WasContactFrom(target_, actor_)) return;
    strikeBonusApplied_ = true;
    const int current = feedingApi_.Health(target_);
    if (current > 0) feedingApi_.SetHealth(target_, std::max(0, current - config_.combat.strikeBonus));
    physicalApi_.ClearContactSource(target_);
}

void VampireCombatController::ApplyFeedResult() noexcept {
    if (feedResultApplied_) return;
    feedResultApplied_ = true;

    const int targetHealth = feedingApi_.Health(target_);
    if (targetHealth > 0 && config_.combat.combatFeedCost > 0) {
        const int floorHealth = role_ == CombatRole::Boss ? 1 : 0;
        feedingApi_.SetHealth(target_, std::max(floorHealth, targetHealth - config_.combat.combatFeedCost));
    }

    const int actorHealth = feedingApi_.Health(actor_);
    const int actorMax = feedingApi_.MaxHealth(actor_);
    if (actorHealth > 0 && actorMax > 0 && config_.combat.combatFeedRestore > 0) {
        feedingApi_.SetHealth(actor_, std::min(actorMax, actorHealth + config_.combat.combatFeedRestore));
    }
    if (role_ == CombatRole::PlayerDebug) {
        feedingController_.GainHiddenResource(config_.combat.combatFeedBloodGain);
    }
}

void VampireCombatController::Enter(CombatState next, std::uint64_t nowMs) noexcept {
    if (config_.debug.enabled && state_ != next) {
        logger_.Write(util::LogLevel::Debug,
            std::string("Combat state ") + StateName(state_) + " -> " + StateName(next) +
            " move=" + MoveName(move_));
    }
    state_ = next;
    stateStartedMs_ = nowMs;
}

void VampireCombatController::Abort(std::string_view reason) noexcept {
    if (state_ == CombatState::Idle) return;
    logger_.Write(util::LogLevel::Warning,
        std::string("Vampire combat aborted safely: ") + std::string(reason) +
        " move=" + MoveName(move_));
    CleanupOwnedState();
    state_ = CombatState::Abort;
    Reset();
}

void VampireCombatController::CleanupOwnedState() noexcept {
    const std::size_t failures = watchdog_.RestoreAll();
    if (failures != 0) logger_.Write(util::LogLevel::Error, "Combat motion cleanup reported a failure.");
    if (actorTaskOwned_ && actor_ != 0 && api_.EntityExists(actor_)) feedingApi_.ClearTasks(actor_);
    if (targetTaskOwned_ && target_ != 0 && api_.EntityExists(target_)) feedingApi_.ClearTasks(target_);
    actorTaskOwned_ = false;
    targetTaskOwned_ = false;
}

void VampireCombatController::Reset() noexcept {
    CleanupOwnedState();
    state_ = CombatState::Idle;
    move_ = CombatMove::HeavyStrike;
    role_ = CombatRole::PlayerDebug;
    actor_ = 0;
    target_ = 0;
    stateStartedMs_ = 0;
    preTelegraphed_ = false;
    strikeBonusApplied_ = false;
    feedResultApplied_ = false;
}

void VampireCombatController::Cancel() noexcept {
    if (state_ != CombatState::Idle) Abort("cancelled");
    else CleanupOwnedState();
}

void VampireCombatController::Shutdown() noexcept { Cancel(); }

int VampireCombatController::WindupMs() const noexcept {
    if (preTelegraphed_) return 0;
    if (move_ == CombatMove::ShadowstepStrike) return config_.combat.shadowstepFollowupWindupMs;
    return config_.combat.heavyWindupMs;
}

bool VampireCombatController::TimedOut(std::uint64_t nowMs) const noexcept {
    return state_ != CombatState::Idle && stateStartedMs_ != 0 && nowMs >= stateStartedMs_ &&
        nowMs - stateStartedMs_ > static_cast<std::uint64_t>(config_.combat.stateTimeoutMs);
}

std::uint64_t VampireCombatController::Elapsed(std::uint64_t nowMs) const noexcept {
    return nowMs >= stateStartedMs_ ? nowMs - stateStartedMs_ : 0;
}

const char* VampireCombatController::StateName(CombatState state) noexcept {
    switch (state) {
        case CombatState::Idle: return "Idle";
        case CombatState::Telegraph: return "Telegraph";
        case CombatState::Align: return "Align";
        case CombatState::Hold: return "Hold";
        case CombatState::Strike: return "Strike";
        case CombatState::Release: return "Release";
        case CombatState::Feed: return "Feed";
        case CombatState::Recover: return "Recover";
        case CombatState::Abort: return "Abort";
        default: return "Unknown";
    }
}

const char* VampireCombatController::MoveName(CombatMove move) noexcept {
    switch (move) {
        case CombatMove::ShadowstepStrike: return "ShadowstepStrike";
        case CombatMove::HeavyStrike: return "HeavyStrike";
        case CombatMove::GrabControl: return "GrabControl";
        case CombatMove::GrabThrow: return "GrabThrow";
        case CombatMove::CombatFeed: return "CombatFeed";
        default: return "Unknown";
    }
}

} // namespace nightwalker::systems
