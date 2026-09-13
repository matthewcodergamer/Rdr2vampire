#include "nightwalker/narrative/ReactiveConversationController.h"

namespace nightwalker::narrative {

ReactiveConversationController::ReactiveConversationController(game::IGameApi& api,
    game::IGameCombatApi& combatApi, game::IGameConversationApi& conversationApi,
    game::IGamePhysicalApi& physicalApi, systems::BossActorRegistry& registry,
    NarrativeController& narrative, util::Logger& logger, const core::Config& config) noexcept
    : api_(api), combatApi_(combatApi), conversationApi_(conversationApi), physicalApi_(physicalApi),
      registry_(registry), narrative_(narrative), logger_(logger), config_(config) {}

bool ReactiveConversationController::Initialize() {
    Cancel();
    logger_.Write(util::LogLevel::Info,
        "Reactive conversation initialized; hold L2/LT to focus, then use TALK, ANTAGONIZE or LEAVE. R2/RT remains hostile fire.");
    return true;
}

bool ReactiveConversationController::BindActor(game::PedHandle actor) noexcept {
    if (actor_ == actor && actor_ != 0) return true;
    ReleaseActor();
    if (actor == 0 || !api_.EntityExists(actor) || !api_.PedAlive(actor)) return false;
    actor_ = actor;
    questionPrompt_ = conversationApi_.CreateEntityPrompt(actor_, "INPUT_CONTEXT_X", "TALK");
    challengePrompt_ = conversationApi_.CreateEntityPrompt(actor_, "INPUT_CONTEXT_Y", "ANTAGONIZE");
    leavePrompt_ = conversationApi_.CreateEntityPrompt(actor_, "INPUT_CONTEXT_B", "LEAVE");
    if (questionPrompt_ == 0 || challengePrompt_ == 0 || leavePrompt_ == 0)
        logger_.Write(util::LogLevel::Warning,
            "A vampire conversation prompt could not be registered; weapon reactions remain active.");
    physicalApi_.ClearContactSource(actor_);
    model_.Reset(); intent_ = ConversationIntent::None;
    return true;
}

void ReactiveConversationController::ReleaseActor() noexcept {
    SetPrompts(false);
    conversationApi_.DeletePrompt(questionPrompt_);
    conversationApi_.DeletePrompt(challengePrompt_);
    conversationApi_.DeletePrompt(leavePrompt_);
    actor_ = 0; model_.Reset(); intent_ = ConversationIntent::None; nextAmbientReactionMs_ = 0;
}

void ReactiveConversationController::SetPrompts(bool enabled) noexcept {
    conversationApi_.SetPromptEnabled(questionPrompt_, enabled);
    conversationApi_.SetPromptEnabled(challengePrompt_, enabled);
    conversationApi_.SetPromptEnabled(leavePrompt_, enabled);
}

void ReactiveConversationController::SetEncounterConversationEnabled(bool enabled) noexcept {
    enabled_ = enabled;
    if (!enabled_) ReleaseActor();
}

ConversationIntent ReactiveConversationController::ConsumeIntent() noexcept {
    const auto result = intent_; intent_ = ConversationIntent::None; return result;
}

void ReactiveConversationController::Cancel() noexcept { enabled_ = false; ReleaseActor(); }
void ReactiveConversationController::Shutdown() noexcept { enabled_ = false; ReleaseActor(); }

} // namespace nightwalker::narrative
