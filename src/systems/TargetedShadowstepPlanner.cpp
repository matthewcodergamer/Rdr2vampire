#include "nightwalker/systems/TargetedShadowstepPlanner.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "nightwalker/systems/ShadowstepMath.h"

namespace nightwalker::systems {
namespace {

constexpr float kMinimumTargetSeparation = 1.10F;
constexpr float kMaximumVerticalPenaltyHeight = 3.0F;

float Dot2D(const game::Vec3& a, const game::Vec3& b) noexcept {
    return a.x * b.x + a.y * b.y;
}

game::Vec3 PredictedTargetPosition(const TargetedShadowstepRequest& request) noexcept {
    game::Vec3 velocityDirection{};
    const float speed = std::sqrt(
        request.targetVelocity.x * request.targetVelocity.x +
        request.targetVelocity.y * request.targetVelocity.y);
    const float displacement = std::min(
        speed * std::max(0.0F, request.predictionSeconds),
        std::max(0.0F, request.maxPredictionMeters));

    if (displacement > 0.01F && shadowstep_math::NormalizeHorizontal(
            request.targetVelocity, velocityDirection)) {
        return shadowstep_math::AddScaled(request.targetPosition, velocityDirection, displacement);
    }
    return request.targetPosition;
}

float PreferenceBonus(ShadowstepCandidateType type, const TargetedShadowstepRequest& request) noexcept {
    if (request.preferEvade) {
        switch (type) {
            case ShadowstepCandidateType::LeftFlank:
            case ShadowstepCandidateType::RightFlank: return 38.0F;
            case ShadowstepCandidateType::Behind: return 20.0F;
            case ShadowstepCandidateType::Intercept: return -20.0F;
        }
    }
    if (request.preferIntercept) {
        switch (type) {
            case ShadowstepCandidateType::Intercept: return 42.0F;
            case ShadowstepCandidateType::LeftFlank:
            case ShadowstepCandidateType::RightFlank: return 18.0F;
            case ShadowstepCandidateType::Behind: return -4.0F;
        }
    }
    switch (type) {
        case ShadowstepCandidateType::LeftFlank:
        case ShadowstepCandidateType::RightFlank: return 24.0F;
        case ShadowstepCandidateType::Behind: return 14.0F;
        case ShadowstepCandidateType::Intercept: return 8.0F;
    }
    return 0.0F;
}

float FacingBonus(
    ShadowstepCandidateType type,
    const game::Vec3& targetForward,
    const game::Vec3& predictedTarget,
    const game::Vec3& candidate) noexcept {
    game::Vec3 targetToCandidate{
        candidate.x - predictedTarget.x,
        candidate.y - predictedTarget.y,
        0.0F,
    };
    game::Vec3 candidateDirection{};
    if (!shadowstep_math::NormalizeHorizontal(targetToCandidate, candidateDirection)) return -50.0F;
    const float facingDot = std::clamp(Dot2D(targetForward, candidateDirection), -1.0F, 1.0F);

    if (type == ShadowstepCandidateType::Behind) return -facingDot * 10.0F;
    if (type == ShadowstepCandidateType::Intercept) return facingDot * 5.0F;
    return (1.0F - std::fabs(facingDot)) * 8.0F;
}

} // namespace

TargetedShadowstepPlan TargetedShadowstepPlanner::Plan(
    const TargetedShadowstepRequest& request) const noexcept {
    TargetedShadowstepPlan plan{};
    if (request.actor == 0 || request.target == 0 || request.actor == request.target) return plan;

    game::Vec3 targetForward{};
    if (!shadowstep_math::NormalizeHorizontal(request.targetForward, targetForward)) {
        game::Vec3 fallback{
            request.actorPosition.x - request.targetPosition.x,
            request.actorPosition.y - request.targetPosition.y,
            0.0F,
        };
        if (!shadowstep_math::NormalizeHorizontal(fallback, targetForward)) return plan;
    }
    const game::Vec3 right{targetForward.y, -targetForward.x, 0.0F};
    const game::Vec3 predicted = PredictedTargetPosition(request);

    game::Vec3 moveDirection{};
    const bool hasMoveDirection = shadowstep_math::NormalizeHorizontal(
        request.targetVelocity, moveDirection);
    const game::Vec3 interceptDirection = hasMoveDirection ? moveDirection : targetForward;
    const float range = std::clamp(request.strikingRange, 1.20F, 4.00F);

    const std::array<std::pair<ShadowstepCandidateType, game::Vec3>, 4> requested{{
        {ShadowstepCandidateType::Intercept, shadowstep_math::AddScaled(predicted, interceptDirection, range)},
        {ShadowstepCandidateType::LeftFlank, shadowstep_math::AddScaled(predicted, right, -range)},
        {ShadowstepCandidateType::RightFlank, shadowstep_math::AddScaled(predicted, right, range)},
        {ShadowstepCandidateType::Behind, shadowstep_math::AddScaled(predicted, targetForward, -range)},
    }};

    for (std::size_t index = 0; index < requested.size(); ++index) {
        auto& evaluation = plan.candidates[index];
        evaluation.type = requested[index].first;
        evaluation.requested = requested[index].second;
        evaluation.resolution = resolver_.ResolveToPoint(
            request.actor,
            request.actorPosition,
            evaluation.requested,
            false);
        if (!evaluation.resolution.valid) continue;

        const float targetDistance = shadowstep_math::Distance2D(
            evaluation.resolution.finalPosition, predicted);
        if (targetDistance < kMinimumTargetSeparation) {
            evaluation.resolution.valid = false;
            evaluation.resolution.reason = ShadowstepRejectReason::ClearanceBlocked;
            continue;
        }

        const float rangePenalty = std::fabs(targetDistance - range) * 28.0F;
        const float verticalDelta = std::min(
            std::fabs(evaluation.resolution.finalPosition.z - predicted.z),
            kMaximumVerticalPenaltyHeight);
        const float verticalPenalty = verticalDelta * 14.0F;
        const float travelPenalty = evaluation.resolution.finalDistance * 0.65F;

        evaluation.score = 100.0F
            + PreferenceBonus(evaluation.type, request)
            + FacingBonus(evaluation.type, targetForward, predicted, evaluation.resolution.finalPosition)
            - rangePenalty
            - verticalPenalty
            - travelPenalty;

        if (!plan.valid || evaluation.score > plan.chosenScore) {
            plan.valid = true;
            plan.chosenType = evaluation.type;
            plan.destination = evaluation.resolution.finalPosition;
            plan.chosenScore = evaluation.score;
        }
    }

    return plan;
}

const char* TargetedShadowstepPlanner::CandidateName(ShadowstepCandidateType type) noexcept {
    switch (type) {
        case ShadowstepCandidateType::Intercept: return "intercept";
        case ShadowstepCandidateType::LeftFlank: return "left-flank";
        case ShadowstepCandidateType::RightFlank: return "right-flank";
        case ShadowstepCandidateType::Behind: return "behind";
        default: return "unknown";
    }
}

} // namespace nightwalker::systems
