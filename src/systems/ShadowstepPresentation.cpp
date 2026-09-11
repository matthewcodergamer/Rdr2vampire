#include "nightwalker/systems/ShadowstepController.h"

#include <algorithm>
#include <string>

#include "nightwalker/systems/ShadowstepMath.h"

namespace nightwalker::systems {
namespace {
constexpr float kCarryChestHeight = 0.85F;
constexpr float kCarryGroundBias = 0.05F;
constexpr float kCarrySafeSnap = 0.65F;
constexpr float kCarryVerificationTolerance = 0.75F;
constexpr float kCarryMaxWaterDepth = 0.35F;
}

void ShadowstepController::ReloadPresentationSettings(const std::filesystem::path& iniPath) noexcept {
    try {
        presentationSettings_ = LoadShadowstepPresentationSettings(
            iniPath,
            [this](std::string_view message) { logger_.Write(util::LogLevel::Warning, message); });
    } catch (...) {
        presentationSettings_ = {};
        logger_.Write(util::LogLevel::Error,
            "Could not load Shadowstep presentation settings; safe defaults restored.");
    }

    carryResolverSettings_ = config_.shadowstep;
    carryResolverSettings_.quickDistance = static_cast<double>(presentationSettings_.carryMeters);
    carryResolverSettings_.maxVerticalDelta =
        std::min(config_.shadowstep.maxVerticalDelta, 0.75);

    if (presentationSettings_.smokeFx) presentationApi_.RequestShadowSmoke();
}

void ShadowstepController::RestorePresentation() noexcept {
    const std::size_t failures = presentationWatchdog_.RestoreAll();
    if (failures != 0) {
        logger_.Write(util::LogLevel::Error,
            "Shadowstep presentation watchdog reported a restoration failure.");
    }
    if (player_ != 0) presentationApi_.RestorePlayerAppearance(player_);
}

void ShadowstepController::SampleMelee(std::uint64_t nowMs) noexcept {
    if (state_ == ShadowstepState::MeleeWindow) return;
    if (!presentationApi_.MeleeInputPressed()) return;
    meleeBuffered_ = true;
    meleeBufferUntilMs_ = nowMs + presentationSettings_.meleeBufferMs;
}

bool ShadowstepController::BeginHiddenTransit(std::uint64_t nowMs) noexcept {
    presentationWatchdog_.RestoreAll();
    const game::PedHandle ped = player_;
    if (!presentationWatchdog_.Own(core::OwnedState::Visibility, [this, ped] {
            presentationApi_.RestorePlayerAppearance(ped);
        })) {
        return false;
    }
    if (!presentationApi_.SetPlayerVisible(player_, false)) {
        presentationWatchdog_.Restore(core::OwnedState::Visibility);
        return false;
    }
    hiddenUntilMs_ = nowMs + presentationSettings_.disappearMs;
    return true;
}

bool ShadowstepController::UpdateArrivalCarry(std::uint64_t nowMs) noexcept {
    if (!carryResolution_.valid || presentationSettings_.carryMeters <= 0.05F) return true;

    const std::uint64_t elapsed = nowMs >= stateStartedMs_ ? nowMs - stateStartedMs_ : 0;
    const float duration = static_cast<float>(std::max<std::uint32_t>(1, presentationSettings_.carryMs));
    const float t = std::clamp(static_cast<float>(elapsed) / duration, 0.0F, 1.0F);
    const game::Vec3 desired{
        carryStart_.x + (carryResolution_.finalPosition.x - carryStart_.x) * t,
        carryStart_.y + (carryResolution_.finalPosition.y - carryStart_.y) * t,
        carryStart_.z + (carryResolution_.finalPosition.z - carryStart_.z) * t,
    };

    const game::Vec3 current = api_.EntityCoords(player_);
    const game::Vec3 traceStart{current.x, current.y, current.z + kCarryChestHeight};
    const game::Vec3 traceEnd{desired.x, desired.y, desired.z + kCarryChestHeight};
    const game::RaycastResult trace = api_.RaycastWorld(traceStart, traceEnd, player_);
    if (!trace.conclusive || trace.hit) {
        logger_.Write(util::LogLevel::Debug,
            "Arrival carry stopped early because the dynamic path was blocked or inconclusive.");
        return true;
    }

    game::Vec3 safePoint{};
    if (!api_.FindSafeCoordForPed(desired, safePoint) ||
        shadowstep_math::Distance2D(desired, safePoint) > kCarrySafeSnap) {
        logger_.Write(util::LogLevel::Debug,
            "Arrival carry stopped early because no nearby safe pedestrian point remained.");
        return true;
    }

    float groundZ = 0.0F;
    if (!api_.TryGroundZ(safePoint, 2.0F, groundZ)) return true;
    game::Vec3 next{safePoint.x, safePoint.y, groundZ + kCarryGroundBias};
    if (!shadowstep_math::VerticalDeltaWithin(
            current, next,
            static_cast<float>(std::min(config_.shadowstep.maxVerticalDelta, 0.75)))) {
        return true;
    }

    float waterHeight = 0.0F;
    if (api_.HasWaterAt(next, waterHeight) && waterHeight - groundZ > kCarryMaxWaterDepth) return true;
    if (!api_.SetEntityCoordsNoOffset(player_, next)) return true;

    const game::Vec3 actual = api_.EntityCoords(player_);
    if (shadowstep_math::Distance3D(actual, next) > kCarryVerificationTolerance) {
        logger_.Write(util::LogLevel::Warning,
            "Arrival carry stopped after relocation verification mismatch; base Shadowstep remains valid.");
        return true;
    }
    return t >= 1.0F;
}

bool ShadowstepController::StateTimedOut(std::uint64_t nowMs) const noexcept {
    if (stateStartedMs_ == 0 || nowMs < stateStartedMs_) return false;
    if (state_ == ShadowstepState::Idle || state_ == ShadowstepState::Cooldown ||
        state_ == ShadowstepState::Error) {
        return false;
    }

    std::uint64_t limit = presentationSettings_.stateTimeoutMs;
    if (state_ == ShadowstepState::HiddenTransit) {
        limit = std::max(limit, static_cast<std::uint64_t>(presentationSettings_.disappearMs + 200));
    } else if (state_ == ShadowstepState::ArrivalCarry) {
        limit = std::max(limit, static_cast<std::uint64_t>(presentationSettings_.carryMs + 250));
    } else if (state_ == ShadowstepState::MeleeWindow) {
        limit = std::max(limit, static_cast<std::uint64_t>(presentationSettings_.meleeBufferMs + 250));
    }
    return nowMs - stateStartedMs_ > limit;
}

} // namespace nightwalker::systems
