#include "nightwalker/narrative/ReactiveConversationController.h"

#include <cmath>

namespace nightwalker::narrative {
namespace {
float Distance3D(const game::Vec3& a, const game::Vec3& b) noexcept {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}
} // namespace

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
    const bool aimed = combatApi_.PlayerAimedPed(player) == actor_;
    const bool shooting = conversationApi_.IsPedShooting(player);
    const bool hitBoss = physicalApi_.WasContactFrom(actor_, player);
    const bool meleeEngaged = conversationApi_.IsMeleeEngagedWith(player, actor_);
    const auto weaponKind = conversationApi_.CurrentWeaponKind(player);
    const float distance = Distance3D(api_.EntityCoords(player), api_.EntityCoords(actor_));

    const bool choicesAvailable = !combat && !narrative_.Active() && !aimed && !shooting && !meleeEngaged;
    SetPrompts(choicesAvailable);

    ReactiveDialogueInput input{};
    input.nowMs = frame.nowMs;
    input.weaponKind = weaponKind;
    input.distanceToBoss = distance;
    input.aimed = !combat && aimed;
    input.shooting = shooting;
    input.hitBoss = hitBoss;
    input.meleeEngaged = meleeEngaged;
    if (choicesAvailable) {
        input.question = conversationApi_.PromptActivated(questionPrompt_);
        input.challenge = conversationApi_.PromptActivated(challengePrompt_);
        input.leave = conversationApi_.PromptActivated(leavePrompt_);
    }

    const auto event = model_.Update(input);
    if (event != ReactiveDialogueEvent::None) HandleEvent(event, frame.nowMs);
}

} // namespace nightwalker::narrative
