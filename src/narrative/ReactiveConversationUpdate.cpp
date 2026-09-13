#include "nightwalker/narrative/ReactiveConversationController.h"

namespace nightwalker::narrative {

void ReactiveConversationController::Update(const core::FrameContext& frame) {
    if (!enabled_ || !config_.IsFeatureEnabled(core::Feature::Narrative) ||
        !registry_.IsOwnedBy(systems::BossOwner::Encounter)) {
        SetPrompts(false);
        return;
    }

    const auto registeredActor = registry_.Ped();
    if (!BindActor(registeredActor)) return;
    if (!api_.EntityExists(actor_) || !api_.PedAlive(actor_)) { ReleaseActor(); return; }

    const auto player = api_.PlayerPed();
    const bool combat = registry_.CombatEnabled();
    if (combat && !model_.ShotPending()) { SetPrompts(false); return; }

    const bool aimed = combatApi_.PlayerAimedPed(player) == actor_;
    const bool shooting = conversationApi_.IsPedShooting(player);
    const bool hitBoss = physicalApi_.WasContactFrom(actor_, player);
    const bool choicesAvailable = !combat && !narrative_.Active() && !aimed && !shooting;
    SetPrompts(choicesAvailable);

    ReactiveDialogueInput input{};
    input.nowMs = frame.nowMs;
    input.aimed = combat ? false : aimed;
    input.shooting = shooting;
    input.hitBoss = hitBoss;
    if (choicesAvailable) {
        input.question = conversationApi_.PromptActivated(questionPrompt_);
        input.challenge = conversationApi_.PromptActivated(challengePrompt_);
        input.leave = conversationApi_.PromptActivated(leavePrompt_);
    }

    const auto event = model_.Update(input);
    if (event != ReactiveDialogueEvent::None) HandleEvent(event, frame.nowMs);
}

} // namespace nightwalker::narrative
