#include "nightwalker/systems/VampireFeedPresentation.h"

#include <algorithm>
#include <cmath>

namespace nightwalker::systems {

FeedPresentationResult VampireFeedPresentation::BeginPaired(game::PedHandle actor,
                                                             game::PedHandle target,
                                                             int durationMs) noexcept {
    if (actor == 0 || target == 0 || actor == target) return {};
    if (!gameApi_.PedAlive(actor) || !gameApi_.PedAlive(target)) return {};

    const bool rear = PreferRearStyle(gameApi_.EntityCoords(actor),
                                      gameApi_.EntityCoords(target),
                                      gameApi_.EntityForward(target));
    const auto style = rear ? game::StandingGrappleStyle::RearControl
                            : game::StandingGrappleStyle::FrontControl;
    if (feedingApi_.StartStyledGrapple(actor, target, style)) {
        return {rear ? FeedPresentationPath::RearGrapple : FeedPresentationPath::FrontGrapple,
                true, true};
    }
    if (feedingApi_.StartGrapple(actor, target)) {
        return {FeedPresentationPath::VanillaGrapple, true, true};
    }
    return BeginStationary(actor, target, durationMs, true);
}

FeedPresentationResult VampireFeedPresentation::BeginStationary(game::PedHandle actor,
                                                                 game::PedHandle target,
                                                                 int durationMs,
                                                                 bool includeTarget) noexcept {
    if (actor == 0 || !gameApi_.EntityExists(actor)) return {};
    feedingApi_.StandStill(actor, std::max(durationMs, 0));
    bool targetOwned = false;
    if (includeTarget && target != 0 && gameApi_.EntityExists(target)) {
        feedingApi_.StandStill(target, std::max(durationMs, 0));
        targetOwned = true;
    }
    return {FeedPresentationPath::StationaryHold, true, targetOwned};
}

bool VampireFeedPresentation::PreferRearStyle(const game::Vec3& actor,
                                               const game::Vec3& target,
                                               const game::Vec3& forward) noexcept {
    const float ax = actor.x - target.x;
    const float ay = actor.y - target.y;
    const float a2 = ax * ax + ay * ay;
    const float f2 = forward.x * forward.x + forward.y * forward.y;
    if (a2 <= 0.0001F || f2 <= 0.0001F) return false;
    const float dot = (ax * forward.x + ay * forward.y) / std::sqrt(a2 * f2);
    return dot <= -0.25F;
}

const char* VampireFeedPresentation::PathName(FeedPresentationPath path) noexcept {
    switch (path) {
        case FeedPresentationPath::RearGrapple: return "rear-grapple";
        case FeedPresentationPath::FrontGrapple: return "front-grapple";
        case FeedPresentationPath::VanillaGrapple: return "vanilla-grapple";
        case FeedPresentationPath::StationaryHold: return "stationary-hold";
        default: return "none";
    }
}

} // namespace nightwalker::systems
