#pragma once

#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameFeedingApi.h"

namespace nightwalker::systems {

enum class FeedPresentationPath {
    None,
    RearGrapple,
    FrontGrapple,
    VanillaGrapple,
    StationaryHold,
};

struct FeedPresentationResult final {
    FeedPresentationPath path{FeedPresentationPath::None};
    bool actorTaskOwned{false};
    bool targetTaskOwned{false};

    [[nodiscard]] bool Started() const noexcept { return path != FeedPresentationPath::None; }
};

class VampireFeedPresentation final {
public:
    VampireFeedPresentation(game::IGameApi& gameApi, game::IGameFeedingApi& feedingApi) noexcept
        : gameApi_(gameApi), feedingApi_(feedingApi) {}

    FeedPresentationResult BeginPaired(game::PedHandle actor,
                                       game::PedHandle target,
                                       int durationMs) noexcept;

    FeedPresentationResult BeginStationary(game::PedHandle actor,
                                           game::PedHandle target,
                                           int durationMs,
                                           bool includeTarget) noexcept;

    [[nodiscard]] static bool PreferRearStyle(const game::Vec3& actorPosition,
                                              const game::Vec3& targetPosition,
                                              const game::Vec3& targetForward) noexcept;
    [[nodiscard]] static const char* PathName(FeedPresentationPath path) noexcept;

private:
    game::IGameApi& gameApi_;
    game::IGameFeedingApi& feedingApi_;
};

} // namespace nightwalker::systems
