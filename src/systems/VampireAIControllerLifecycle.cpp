#include "nightwalker/systems/VampireAIController.h"

#include <algorithm>
#include <string>

namespace nightwalker::systems {

bool VampireAIController::ValidCombatActors() const noexcept {
 return vampire_!=0&&player_!=0&&vampire_==spawner_.OwnedPed()&&api_.EntityExists(vampire_)&&api_.EntityModel(vampire_)==DebugVampireSpawner::kVampireModel&&api_.PedAlive(vampire_)&&api_.PedAlive(player_);
}

void VampireAIController::EnsureOrdinaryCombat() noexcept {
 if(ValidCombatActors()&&!combatController_.IsActiveFor(vampire_))combatApi_.TaskCombatPed(vampire_,player_);
}

CombatMove VampireAIController::NextCloseCombatMove() noexcept {
 const unsigned slot=bossSpecialSequence_++%3U;
 if(slot==0U)return CombatMove::HeavyStrike;
 if(slot==1U)return CombatMove::GrabThrow;
 return CombatMove::CombatFeed;
}

void VampireAIController::LogPlan(const TargetedShadowstepPlan& plan) const {
 if(!config_.debug.enabled)return;
 for(const auto& candidate:plan.candidates){if(candidate.resolution.valid)logger_.Write(util::LogLevel::Debug,std::string("Vampire candidate ")+TargetedShadowstepPlanner::CandidateName(candidate.type)+" score="+std::to_string(candidate.score)+" distance="+std::to_string(candidate.resolution.finalDistance));else logger_.Write(util::LogLevel::Debug,std::string("Vampire candidate ")+TargetedShadowstepPlanner::CandidateName(candidate.type)+" rejected="+ShadowstepResolver::ReasonText(candidate.resolution.reason));}
 if(plan.valid)logger_.Write(util::LogLevel::Debug,std::string("Vampire chose ")+TargetedShadowstepPlanner::CandidateName(plan.chosenType)+" score="+std::to_string(plan.chosenScore));else logger_.Write(util::LogLevel::Debug,"Vampire found no safe Shadowstep candidate; ordinary combat retained.");
}

void VampireAIController::RestoreOwnedPresentation() noexcept {
 const std::size_t failures=presentationWatchdog_.RestoreAll();if(failures!=0)logger_.Write(util::LogLevel::Error,"Vampire Shadowstep presentation cleanup reported a failure.");
 if(vampire_!=0&&api_.EntityExists(vampire_)&&api_.EntityModel(vampire_)==DebugVampireSpawner::kVampireModel)presentationApi_.RestorePedAppearance(vampire_);
}

void VampireAIController::Abort(const char* reason,std::uint64_t nowMs) noexcept {
 logger_.Write(util::LogLevel::Warning,std::string("Vampire AI aborted safely: ")+reason+" state="+StateName(state_));RestoreOwnedPresentation();combatController_.CancelForActor(vampire_);
 if(vampire_!=0&&vampire_==spawner_.OwnedPed()&&api_.EntityExists(vampire_)&&api_.EntityModel(vampire_)==DebugVampireSpawner::kVampireModel)combatApi_.ClearPedTasks(vampire_);
 Transition(VampireAiState::Abort,nowMs);
}

void VampireAIController::Cancel() noexcept {
 const game::PedHandle owned=spawner_.OwnedPed();combatController_.CancelForActor(owned);RestoreOwnedPresentation();
 if(owned!=0&&api_.EntityExists(owned)&&api_.EntityModel(owned)==DebugVampireSpawner::kVampireModel){combatApi_.ClearPedTasks(owned);presentationApi_.RestorePedAppearance(owned);}
 state_=VampireAiState::Observe;stateStartedMs_=0;ResetTransient();
}

void VampireAIController::Shutdown() noexcept {Cancel();presentationApi_.ReleaseShadowSmoke();}

void VampireAIController::ResetTransient() noexcept {
 vampire_=0;player_=0;plan_={};departPosition_={};carryStart_={};carryDestination_={};hiddenUntilMs_=0;nextDecisionMs_=0;shadowstepCooldownUntilMs_=0;evadeCooldownUntilMs_=0;bossSpecialCooldownUntilMs_=0;bossSpecialSequence_=0;evadeIntent_=false;evadeOpportunityToggle_=false;playerMeleeWasDown_=false;
}

void VampireAIController::Transition(VampireAiState next,std::uint64_t nowMs) noexcept {
 if(config_.debug.enabled&&state_!=next){const std::uint64_t elapsed=stateStartedMs_==0?0:nowMs-stateStartedMs_;logger_.Write(util::LogLevel::Debug,std::string("Vampire AI state ")+StateName(state_)+" -> "+StateName(next)+" elapsedMs="+std::to_string(elapsed));}
 state_=next;stateStartedMs_=nowMs;
}

bool VampireAIController::StateTimedOut(std::uint64_t nowMs) const noexcept {
 if(stateStartedMs_==0||nowMs<stateStartedMs_)return false;
 switch(state_){case VampireAiState::Observe:case VampireAiState::Approach:case VampireAiState::Cooldown:case VampireAiState::Abort:return false;default:break;}
 std::uint64_t limit=presentationSettings_.stateTimeoutMs;
 if(state_==VampireAiState::HiddenTransit)limit=std::max(limit,static_cast<std::uint64_t>(presentationSettings_.disappearMs+250U));
 else if(state_==VampireAiState::ShadowstepArrive)limit=std::max(limit,static_cast<std::uint64_t>(presentationSettings_.carryMs+300U));
 else if(state_==VampireAiState::Telegraph)limit=std::max(limit,static_cast<std::uint64_t>(config_.vampireAi.telegraphMs+300));
 else if(state_==VampireAiState::Attack||state_==VampireAiState::MeleeAbility)limit=std::max(limit,static_cast<std::uint64_t>(config_.combat.stateTimeoutMs+config_.combat.recoveryMs+500));
 return nowMs-stateStartedMs_>limit;
}

const char* VampireAIController::StateName(VampireAiState state) noexcept {
 switch(state){case VampireAiState::Observe:return "Observe";case VampireAiState::Approach:return "Approach";case VampireAiState::Decide:return "Decide";case VampireAiState::ShadowstepDepart:return "ShadowstepDepart";case VampireAiState::HiddenTransit:return "HiddenTransit";case VampireAiState::ShadowstepArrive:return "ShadowstepArrive";case VampireAiState::Telegraph:return "Telegraph";case VampireAiState::Attack:return "Attack";case VampireAiState::MeleeAbility:return "MeleeAbility";case VampireAiState::Recover:return "Recover";case VampireAiState::Cooldown:return "Cooldown";case VampireAiState::Evade:return "Evade";case VampireAiState::Reposition:return "Reposition";case VampireAiState::FeedAttempt:return "FeedAttempt";case VampireAiState::Abort:return "Abort";default:return "Unknown";}
}

} // namespace nightwalker::systems
