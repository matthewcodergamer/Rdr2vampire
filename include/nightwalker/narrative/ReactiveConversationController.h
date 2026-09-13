#pragma once

#include <cstdint>
#include <string_view>

#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameCombatApi.h"
#include "nightwalker/game/GameConversationApi.h"
#include "nightwalker/game/GamePhysicalApi.h"
#include "nightwalker/narrative/NarrativeController.h"
#include "nightwalker/narrative/ReactiveDialogueModel.h"
#include "nightwalker/systems/BossActorRegistry.h"
#include "nightwalker/util/Logger.h"

namespace nightwalker::narrative {

enum class ConversationIntent { None, Challenge, Leave, Hostile };

class ReactiveConversationController final : public core::ILifecycleSystem {
public:
    ReactiveConversationController(game::IGameApi& api, game::IGameCombatApi& combatApi,
        game::IGameConversationApi& conversationApi, game::IGamePhysicalApi& physicalApi,
        systems::BossActorRegistry& registry, NarrativeController& narrative,
        util::Logger& logger, const core::Config& config) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept override { return "ReactiveConversationController"; }
    bool Initialize() override;
    void Update(const core::FrameContext& frame) override;
    void Cancel() noexcept override;
    void Shutdown() noexcept override;

    ConversationIntent ConsumeIntent() noexcept;

private:
    bool BindActor(game::PedHandle actor) noexcept;
    void ReleaseActor() noexcept;
    void SetPrompts(bool enabled) noexcept;
    bool StartFamily(std::string_view family, std::uint64_t nowMs, bool interrupt) noexcept;
    void HandleEvent(ReactiveDialogueEvent event, std::uint64_t nowMs) noexcept;

    game::IGameApi& api_;
    game::IGameCombatApi& combatApi_;
    game::IGameConversationApi& conversationApi_;
    game::IGamePhysicalApi& physicalApi_;
    systems::BossActorRegistry& registry_;
    NarrativeController& narrative_;
    util::Logger& logger_;
    const core::Config& config_;
    ReactiveDialogueModel model_{};
    game::PedHandle actor_{0};
    game::PromptHandle questionPrompt_{0};
    game::PromptHandle challengePrompt_{0};
    game::PromptHandle leavePrompt_{0};
    ConversationIntent intent_{ConversationIntent::None};
    std::uint64_t nextAmbientReactionMs_{0};
};

} // namespace nightwalker::narrative
