#include "nightwalker/systems/ShadowstepResolver.h"

#include <array>
#include <cmath>

#include "nightwalker/systems/ShadowstepMath.h"

namespace nightwalker::systems {
namespace {

constexpr float kMinimumStepDistance = 1.25F;
constexpr float kMaximumSafePointSnap = 1.25F;
constexpr float kMaximumWaterDepth = 0.35F;
constexpr float kGroundBias = 0.05F;
constexpr float kChestHeight = 0.85F;
constexpr float kHeadStartHeight = 0.30F;
constexpr float kHeadEndHeight = 1.85F;

} // namespace

ShadowstepResolver::ShadowstepResolver(game::IGameApi& api, const core::ShadowstepSettings& settings) noexcept
    : api_(api), settings_(settings) {}

ShadowstepResolveResult ShadowstepResolver::Resolve(
    game::PedHandle actor,
    const game::Vec3& start,
    const game::Vec3& desiredDirection) const noexcept {
    game::Vec3 direction{};
    ShadowstepResolveResult invalid{};
    invalid.requestedDistance = static_cast<float>(settings_.quickDistance);
    if (!shadowstep_math::NormalizeHorizontal(desiredDirection, direction)) {
        invalid.reason = ShadowstepRejectReason::InvalidDirection;
        return invalid;
    }

    const game::Vec3 requested = shadowstep_math::AddScaled(
        start, direction, static_cast<float>(settings_.quickDistance));
    return ResolveToPoint(actor, start, requested, true);
}

ShadowstepResolveResult ShadowstepResolver::ResolveToPoint(
    game::PedHandle actor,
    const game::Vec3& start,
    const game::Vec3& desiredPoint,
    bool allowShorten) const noexcept {
    ShadowstepResolveResult result{};
    result.requested = desiredPoint;
    result.requestedDistance = shadowstep_math::Distance3D(start, desiredPoint);

    if (!api_.PedAlive(actor)) {
        result.reason = ShadowstepRejectReason::InvalidPlayer;
        return result;
    }

    game::Vec3 direction{};
    const game::Vec3 delta{
        desiredPoint.x - start.x,
        desiredPoint.y - start.y,
        desiredPoint.z - start.z,
    };
    if (!shadowstep_math::NormalizeHorizontal(delta, direction)) {
        result.reason = ShadowstepRejectReason::InvalidDirection;
        return result;
    }

    game::Vec3 candidate = desiredPoint;
    const game::Vec3 traceStart{start.x, start.y, start.z + kChestHeight};
    const game::Vec3 traceEnd{desiredPoint.x, desiredPoint.y, desiredPoint.z + kChestHeight};
    const game::RaycastResult pathTrace = api_.RaycastWorld(traceStart, traceEnd, actor);
    if (!pathTrace.conclusive) {
        result.reason = ShadowstepRejectReason::TraceInconclusive;
        return result;
    }

    if (pathTrace.hit) {
        if (!allowShorten) {
            result.reason = ShadowstepRejectReason::ObstructedTooClose;
            return result;
        }
        result.shortened = true;
        candidate = shadowstep_math::PullBackFromHit(
            {pathTrace.endCoords.x, pathTrace.endCoords.y, start.z},
            direction,
            static_cast<float>(settings_.wallClearance));
        if (shadowstep_math::Distance2D(start, candidate) < kMinimumStepDistance) {
            result.reason = ShadowstepRejectReason::ObstructedTooClose;
            return result;
        }
    }

    game::Vec3 safePoint{};
    if (!api_.FindSafeCoordForPed(candidate, safePoint)) {
        result.reason = ShadowstepRejectReason::UnsafeNavmesh;
        return result;
    }
    if (shadowstep_math::Distance2D(candidate, safePoint) > kMaximumSafePointSnap) {
        result.reason = ShadowstepRejectReason::SafePointTooFar;
        return result;
    }

    float groundZ = 0.0F;
    const float probeHeight = static_cast<float>(settings_.maxVerticalDelta) + 2.0F;
    if (!api_.TryGroundZ(safePoint, probeHeight, groundZ)) {
        result.reason = ShadowstepRejectReason::GroundUnavailable;
        return result;
    }

    game::Vec3 finalPosition{safePoint.x, safePoint.y, groundZ + kGroundBias};
    if (!shadowstep_math::VerticalDeltaWithin(
            start, finalPosition, static_cast<float>(settings_.maxVerticalDelta))) {
        result.reason = ShadowstepRejectReason::VerticalDeltaTooLarge;
        return result;
    }

    float waterHeight = 0.0F;
    if (api_.HasWaterAt(finalPosition, waterHeight) && waterHeight - groundZ > kMaximumWaterDepth) {
        result.reason = ShadowstepRejectReason::DeepWater;
        return result;
    }

    ShadowstepRejectReason clearanceReason = ShadowstepRejectReason::None;
    if (!HasClearance(actor, finalPosition, clearanceReason)) {
        result.reason = clearanceReason;
        return result;
    }

    result.finalPosition = finalPosition;
    result.finalDistance = shadowstep_math::Distance3D(start, finalPosition);
    result.valid = true;
    result.reason = ShadowstepRejectReason::None;
    return result;
}

bool ShadowstepResolver::HasClearance(
    game::PedHandle actor,
    const game::Vec3& position,
    ShadowstepRejectReason& rejection) const noexcept {
    const float clearance = static_cast<float>(settings_.wallClearance);
    const game::Vec3 chest{position.x, position.y, position.z + kChestHeight};
    constexpr std::array<game::Vec3, 4> directions{{
        {1.0F, 0.0F, 0.0F}, {-1.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}, {0.0F, -1.0F, 0.0F},
    }};

    for (const auto& direction : directions) {
        const game::Vec3 end = shadowstep_math::AddScaled(chest, direction, clearance);
        const game::RaycastResult trace = api_.RaycastWorld(chest, end, actor);
        if (!trace.conclusive) {
            rejection = ShadowstepRejectReason::TraceInconclusive;
            return false;
        }
        if (trace.hit) {
            rejection = ShadowstepRejectReason::ClearanceBlocked;
            return false;
        }
    }

    const game::Vec3 headStart{position.x, position.y, position.z + kHeadStartHeight};
    const game::Vec3 headEnd{position.x, position.y, position.z + kHeadEndHeight};
    const game::RaycastResult headTrace = api_.RaycastWorld(headStart, headEnd, actor);
    if (!headTrace.conclusive) {
        rejection = ShadowstepRejectReason::TraceInconclusive;
        return false;
    }
    if (headTrace.hit) {
        rejection = ShadowstepRejectReason::HeadroomBlocked;
        return false;
    }

    rejection = ShadowstepRejectReason::None;
    return true;
}

const char* ShadowstepResolver::ReasonText(ShadowstepRejectReason reason) noexcept {
    switch (reason) {
        case ShadowstepRejectReason::None: return "none";
        case ShadowstepRejectReason::InvalidPlayer: return "invalid actor";
        case ShadowstepRejectReason::InvalidDirection: return "invalid direction";
        case ShadowstepRejectReason::TraceInconclusive: return "geometry trace inconclusive";
        case ShadowstepRejectReason::ObstructedTooClose: return "obstruction too close";
        case ShadowstepRejectReason::UnsafeNavmesh: return "no safe navmesh point";
        case ShadowstepRejectReason::SafePointTooFar: return "safe point snapped too far";
        case ShadowstepRejectReason::GroundUnavailable: return "ground unavailable";
        case ShadowstepRejectReason::VerticalDeltaTooLarge: return "vertical delta too large";
        case ShadowstepRejectReason::DeepWater: return "deep water";
        case ShadowstepRejectReason::ClearanceBlocked: return "destination clearance blocked";
        case ShadowstepRejectReason::HeadroomBlocked: return "destination headroom blocked";
        default: return "unknown rejection";
    }
}

} // namespace nightwalker::systems
