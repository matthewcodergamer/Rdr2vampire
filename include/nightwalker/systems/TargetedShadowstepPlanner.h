#pragma once

#include <array>

#include "nightwalker/game/GameApi.h"
#include "nightwalker/systems/ShadowstepResolver.h"

namespace nightwalker::systems {

enum class ShadowstepCandidateType {
    Intercept,
    LeftFlank,
    RightFlank,
    Behind,
};

struct ShadowstepCandidateEvaluation final {
    ShadowstepCandidateType type{ShadowstepCandidateType::Intercept};
    game::Vec3 requested{};
    ShadowstepResolveResult resolution{};
    float score{-100000.0F};
};

struct TargetedShadowstepRequest final {
    game::PedHandle actor{0};
    game::PedHandle target{0};
    game::Vec3 actorPosition{};
    game::Vec3 targetPosition{};
    game::Vec3 targetForward{};
    game::Vec3 targetVelocity{};
    float strikingRange{1.65F};
    float predictionSeconds{0.25F};
    float maxPredictionMeters{1.75F};
    bool preferIntercept{false};
    bool preferEvade{false};
};

struct TargetedShadowstepPlan final {
    bool valid{false};
    ShadowstepCandidateType chosenType{ShadowstepCandidateType::Intercept};
    game::Vec3 destination{};
    float chosenScore{-100000.0F};
    std::array<ShadowstepCandidateEvaluation, 4> candidates{};
};

class TargetedShadowstepPlanner final {
public:
    explicit TargetedShadowstepPlanner(ShadowstepResolver& resolver) noexcept : resolver_(resolver) {}

    [[nodiscard]] TargetedShadowstepPlan Plan(const TargetedShadowstepRequest& request) const noexcept;
    [[nodiscard]] static const char* CandidateName(ShadowstepCandidateType type) noexcept;

private:
    ShadowstepResolver& resolver_;
};

} // namespace nightwalker::systems
