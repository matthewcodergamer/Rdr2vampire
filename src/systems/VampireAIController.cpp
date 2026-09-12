#include "nightwalker/systems/VampireAIController.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "nightwalker/systems/ShadowstepMath.h"

namespace nightwalker::systems {
namespace {
constexpr float kRelocationVerificationTolerance = 1.0F;
constexpr float kEvadeTriggerDistance = 3.4F;
constexpr float kRetreatDotThreshold = 0.35F;
constexpr float kMinimumArrivalTargetDistance = 1.05F;
float HorizontalSpeed(const game::Vec3& v) noexcept { return std::sqrt(v.x*v.x+v.y*v.y); }
float Dot2D(const game::Vec3& a,const game::Vec3& b) noexcept { return a.x*b.x+a.y*b.y; }
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
 if(!config_.IsFeatureEnabled(core::Feature::VampireAi)||!config_.debug.enabled){if(state_!=VampireAiState::Observe||vampire_!=0)Cancel();return;}
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
    if(combatController_.RequestBoss(move,vampire_,player_,frame.nowMs,false)){
     Transition(VampireAiState::MeleeAbility,frame.nowMs);return;
    }
   }

   if(frame.nowMs<shadowstepCooldownUntilMs_){EnsureOrdinaryCombat();return;}
   if(distance>=static_cast<float>(config_.vampireAi.shadowstepMinDistance)&&distance<=static_cast<float>(config_.vampireAi.shadowstepMaxDistance)){
    evadeIntent_=false;Transition(VampireAiState::Decide,frame.nowMs);return;
   }
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
     if(carryPlan.valid&&shadowstep_math::Distance2D(carryPlan.finalPosition,playerPos)>=kMinimumArrivalTargetDistance)carryDestination_=carryPlan.finalPosition;
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

bool VampireAIController::BeginShadowstep(std::uint64_t nowMs) noexcept {
 if(!ValidCombatActors()||!plan_.valid)return false;departPosition_=api_.EntityCoords(vampire_);if(presentationSettings_.smokeFx)presentationApi_.PlayShadowSmoke(departPosition_,0.86F);
 presentationWatchdog_.RestoreAll();const game::PedHandle ownedPed=vampire_;
 if(!presentationWatchdog_.Own(core::OwnedState::Visibility,[this,ownedPed]{presentationApi_.RestorePedAppearance(ownedPed);} ))return false;
 if(!presentationApi_.SetPedVisible(vampire_,false)){presentationWatchdog_.Restore(core::OwnedState::Visibility);return false;}
 combatApi_.TaskStandStill(vampire_,static_cast<int>(presentationSettings_.disappearMs+100U));
 if(!api_.SetEntityCoordsNoOffset(vampire_,plan_.destination)){RestoreOwnedPresentation();return false;}
 const game::Vec3 actual=api_.EntityCoords(vampire_);if(shadowstep_math::Distance3D(actual,plan_.destination)>kRelocationVerificationTolerance){if(api_.PedAlive(vampire_))api_.SetEntityCoordsNoOffset(vampire_,departPosition_);RestoreOwnedPresentation();return false;}
 hiddenUntilMs_=nowMs+presentationSettings_.disappearMs;Transition(VampireAiState::HiddenTransit,nowMs);return true;
}

bool VampireAIController::UpdateArrivalCarry(std::uint64_t nowMs) noexcept {
 if(shadowstep_math::Distance2D(carryStart_,carryDestination_)<=0.05F)return true;
 const std::uint64_t elapsed=nowMs>=stateStartedMs_?nowMs-stateStartedMs_:0;const float duration=static_cast<float>(std::max<std::uint32_t>(1,presentationSettings_.carryMs));const float t=std::clamp(static_cast<float>(elapsed)/duration,0.0F,1.0F);
 const game::Vec3 desired{carryStart_.x+(carryDestination_.x-carryStart_.x)*t,carryStart_.y+(carryDestination_.y-carryStart_.y)*t,carryStart_.z+(carryDestination_.z-carryStart_.z)*t};
 const game::Vec3 current=api_.EntityCoords(vampire_);const auto safe=resolver_.ResolveToPoint(vampire_,current,desired,false);
 if(!safe.valid){logger_.Write(util::LogLevel::Debug,std::string("Vampire arrival carry stopped early: ")+ShadowstepResolver::ReasonText(safe.reason));return true;}
 if(!api_.SetEntityCoordsNoOffset(vampire_,safe.finalPosition))return true;return t>=1.0F;
}

bool VampireAIController::PlayerRetreating() const noexcept {
 if(!ValidCombatActors())return false;const game::Vec3 velocity=combatApi_.EntityVelocity(player_);if(HorizontalSpeed(velocity)<static_cast<float>(config_.vampireAi.retreatSpeedThreshold))return false;
 game::Vec3 moveDirection{};if(!shadowstep_math::NormalizeHorizontal(velocity,moveDirection))return false;const game::Vec3 vampirePos=api_.EntityCoords(vampire_),playerPos=api_.EntityCoords(player_);game::Vec3 away{playerPos.x-vampirePos.x,playerPos.y-vampirePos.y,0.0F},awayDirection{};if(!shadowstep_math::NormalizeHorizontal(away,awayDirection))return false;return Dot2D(moveDirection,awayDirection)>=kRetreatDotThreshold;
}

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
 vampire_=0;player_=0;plan_={};departPosition_={};carryStart_={};carryDestination_={};hiddenUntilMs_=0;nextDecisionMs_=0;shadowstepCooldownUntilMs_=0;evadeCooldownUntilMs_=0;bossSpecialCooldownUntilMs_=0;bossSpecialSequence_=0;evadeIntent_=false;playerMeleeWasDown_=false;
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
