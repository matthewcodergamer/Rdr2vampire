#include "nightwalker/systems/VampireAIController.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "nightwalker/systems/ShadowstepMath.h"

namespace nightwalker::systems {
namespace {
constexpr float kEvadeTriggerDistance = 3.4F;
}

VampireAIController::VampireAIController(
    game::IGameApi& api,
    game::IGameCombatApi& combatApi,
    game::IGamePresentationApi& presentationApi,
    DebugVampireSpawner& spawner,
    VampireCombatController& combatController,
    util::Logger& logger,
    const core::Config& config) noexcept
    : api_(api),combatApi_(combatApi),presentationApi_(presentationApi),spawner_(spawner),
      combatController_(combatController),logger_(logger),config_(config),
      resolver_(api,config.shadowstep),planner_(resolver_) {}

bool VampireAIController::Initialize(){state_=VampireAiState::Observe;ResetTransient();if(presentationSettings_.smokeFx)presentationApi_.RequestShadowSmoke();return true;}

void VampireAIController::ReloadPresentationSettings(const std::filesystem::path& iniPath) noexcept {
 try{presentationSettings_=LoadShadowstepPresentationSettings(iniPath,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);});}
 catch(...){presentationSettings_={};logger_.Write(util::LogLevel::Error,"Vampire AI presentation settings failed to load; safe defaults restored.");}
 if(presentationSettings_.smokeFx)presentationApi_.RequestShadowSmoke();
}

void VampireAIController::Update(const core::FrameContext& frame){
 if(!config_.IsFeatureEnabled(core::Feature::VampireAi)||!spawner_.CombatEnabled()){if(state_!=VampireAiState::Observe||vampire_!=0)Cancel();return;}
 if(StateTimedOut(frame.nowMs)){Abort("state watchdog timeout",frame.nowMs);return;}

 switch(state_){
  case VampireAiState::Observe:{
   vampire_=spawner_.OwnedPed();player_=api_.PlayerPed();
   if(!ValidCombatActors()){vampire_=0;player_=0;return;}
   EnsureOrdinaryCombat();nextDecisionMs_=frame.nowMs;Transition(VampireAiState::Approach,frame.nowMs);return;
  }
  case VampireAiState::Approach:{
   if(!ValidCombatActors()){Abort("combat actor became invalid",frame.nowMs);return;}
   const bool meleeDown=presentationApi_.MeleeInputPressed();const bool meleePressed=meleeDown&&!playerMeleeWasDown_;playerMeleeWasDown_=meleeDown;
   const float distance=shadowstep_math::Distance2D(api_.EntityCoords(vampire_),api_.EntityCoords(player_));
   if(meleePressed&&distance<=kEvadeTriggerDistance&&frame.nowMs>=evadeCooldownUntilMs_){evadeOpportunityToggle_=!evadeOpportunityToggle_;if(evadeOpportunityToggle_){evadeIntent_=true;Transition(VampireAiState::Evade,frame.nowMs);return;}}
   if(frame.nowMs<nextDecisionMs_)return;
   nextDecisionMs_=frame.nowMs+static_cast<std::uint64_t>(config_.vampireAi.decisionIntervalMs);
   if(config_.IsFeatureEnabled(core::Feature::Combat)&&distance<=static_cast<float>(config_.combat.maxDistance)&&frame.nowMs>=bossSpecialCooldownUntilMs_&&!combatController_.IsActive()){
    const CombatMove move=NextCloseCombatMove();
    if(combatController_.RequestBoss(move,vampire_,player_,frame.nowMs,false)){Transition(VampireAiState::MeleeAbility,frame.nowMs);return;}
   }
   if(frame.nowMs<shadowstepCooldownUntilMs_){EnsureOrdinaryCombat();return;}
   if(distance>=static_cast<float>(config_.vampireAi.shadowstepMinDistance)&&distance<=static_cast<float>(config_.vampireAi.shadowstepMaxDistance)){evadeIntent_=false;Transition(VampireAiState::Decide,frame.nowMs);return;}
   EnsureOrdinaryCombat();return;
  }
  case VampireAiState::Evade:
   if(!ValidCombatActors()){Abort("evade actors invalid",frame.nowMs);return;}
   evadeIntent_=true;Transition(VampireAiState::Decide,frame.nowMs);return;
  case VampireAiState::Decide:{
   if(!ValidCombatActors()){Abort("decision actors invalid",frame.nowMs);return;}
   const bool retreating=!evadeIntent_&&PlayerRetreating();
   const float planningRange=static_cast<float>(config_.vampireAi.strikingRange)+(evadeIntent_?0.0F:presentationSettings_.carryMeters);
   TargetedShadowstepRequest request{};request.actor=vampire_;request.target=player_;request.actorPosition=api_.EntityCoords(vampire_);request.targetPosition=api_.EntityCoords(player_);request.targetForward=api_.EntityForward(player_);request.targetVelocity=combatApi_.EntityVelocity(player_);request.strikingRange=planningRange;request.predictionSeconds=static_cast<float>(config_.vampireAi.predictionMs)/1000.0F;request.maxPredictionMeters=1.75F;request.preferIntercept=retreating;request.preferEvade=evadeIntent_;
   plan_=planner_.Plan(request);LogPlan(plan_);
   if(!plan_.valid){if(evadeIntent_)evadeCooldownUntilMs_=frame.nowMs+static_cast<std::uint64_t>(config_.vampireAi.evadeCooldownMs);evadeIntent_=false;EnsureOrdinaryCombat();nextDecisionMs_=frame.nowMs+static_cast<std::uint64_t>(config_.vampireAi.decisionIntervalMs);Transition(VampireAiState::Approach,frame.nowMs);return;}
   lastCandidate_=plan_.chosenType;Transition(VampireAiState::ShadowstepDepart,frame.nowMs);return;
  }
  case VampireAiState::ShadowstepDepart:
   if(!BeginShadowstep(frame.nowMs))Abort("could not begin owned vampire Shadowstep",frame.nowMs);return;
  case VampireAiState::HiddenTransit:{
   if(!ValidCombatActors()){Abort("combat actor invalid during hidden transit",frame.nowMs);return;}
   if(frame.nowMs<hiddenUntilMs_)return;
   RestoreOwnedPresentation();if(presentationSettings_.smokeFx)presentationApi_.PlayShadowSmoke(api_.EntityCoords(vampire_),1.08F);
   carryStart_=api_.EntityCoords(vampire_);carryDestination_=carryStart_;
   if(!evadeIntent_&&presentationSettings_.carryMeters>0.05F){
    const game::Vec3 playerPos=api_.EntityCoords(player_);const float currentDistance=shadowstep_math::Distance2D(carryStart_,playerPos);const float availableCarry=std::max(0.0F,currentDistance-static_cast<float>(config_.vampireAi.strikingRange));const float carryDistance=std::min(presentationSettings_.carryMeters,availableCarry);
    game::Vec3 toPlayer{playerPos.x-carryStart_.x,playerPos.y-carryStart_.y,0.0F};game::Vec3 direction{};
    if(carryDistance>0.05F&&shadowstep_math::NormalizeHorizontal(toPlayer,direction)){
     const game::Vec3 desired=shadowstep_math::AddScaled(carryStart_,direction,carryDistance);const auto carryPlan=resolver_.ResolveToPoint(vampire_,carryStart_,desired,false);
     if(carryPlan.valid&&shadowstep_math::Distance2D(carryPlan.finalPosition,playerPos)>=1.05F)carryDestination_=carryPlan.finalPosition;
     else if(config_.debug.enabled)logger_.Write(util::LogLevel::Debug,std::string("Vampire arrival carry skipped: ")+ShadowstepResolver::ReasonText(carryPlan.reason));
    }
   }
   Transition(VampireAiState::ShadowstepArrive,frame.nowMs);return;
  }
  case VampireAiState::ShadowstepArrive:
   if(!ValidCombatActors()){Abort("combat actor invalid during arrival",frame.nowMs);return;}
   if(UpdateArrivalCarry(frame.nowMs)){combatApi_.TaskStandStill(vampire_,config_.vampireAi.telegraphMs);Transition(VampireAiState::Telegraph,frame.nowMs);}return;
  case VampireAiState::Telegraph:
   if(!ValidCombatActors()){Abort("combat actor invalid during telegraph",frame.nowMs);return;}
   if(frame.nowMs-stateStartedMs_<static_cast<std::uint64_t>(config_.vampireAi.telegraphMs))return;
   if(!combatController_.RequestBoss(CombatMove::ShadowstepStrike,vampire_,player_,frame.nowMs,true))combatApi_.TaskCombatPed(vampire_,player_);
   Transition(VampireAiState::Attack,frame.nowMs);return;
  case VampireAiState::Attack:
   if(!ValidCombatActors()){Abort("combat actor invalid during attack",frame.nowMs);return;}
   if(combatController_.IsActiveFor(vampire_))return;
   if(frame.nowMs-stateStartedMs_<static_cast<std::uint64_t>(config_.vampireAi.recoveryMs))return;
   Transition(VampireAiState::Recover,frame.nowMs);return;
  case VampireAiState::MeleeAbility:
   if(!ValidCombatActors()){Abort("close combat actor invalid",frame.nowMs);return;}
   if(combatController_.IsActiveFor(vampire_))return;
   bossSpecialCooldownUntilMs_=frame.nowMs+static_cast<std::uint64_t>(config_.combat.bossSpecialCooldownMs);
   EnsureOrdinaryCombat();nextDecisionMs_=frame.nowMs+static_cast<std::uint64_t>(config_.vampireAi.decisionIntervalMs);Transition(VampireAiState::Approach,frame.nowMs);return;
  case VampireAiState::Recover:
   shadowstepCooldownUntilMs_=frame.nowMs+static_cast<std::uint64_t>(config_.vampireAi.shadowstepCooldownMs);
   if(evadeIntent_)evadeCooldownUntilMs_=frame.nowMs+static_cast<std::uint64_t>(config_.vampireAi.evadeCooldownMs);
   evadeIntent_=false;plan_={};EnsureOrdinaryCombat();Transition(VampireAiState::Cooldown,frame.nowMs);return;
  case VampireAiState::Cooldown:
   if(!ValidCombatActors()){Abort("combat actor invalid during cooldown",frame.nowMs);return;}
   if(frame.nowMs>=shadowstepCooldownUntilMs_){nextDecisionMs_=frame.nowMs;Transition(VampireAiState::Approach,frame.nowMs);}return;
  case VampireAiState::Reposition:
  case VampireAiState::FeedAttempt:
   Transition(VampireAiState::Approach,frame.nowMs);return;
  case VampireAiState::Abort:
   RestoreOwnedPresentation();ResetTransient();state_=VampireAiState::Observe;stateStartedMs_=frame.nowMs;return;
 }
}

} // namespace nightwalker::systems
