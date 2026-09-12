#include "nightwalker/systems/FeedingController.h"
#include "nightwalker/systems/FeedingMath.h"
#include <algorithm>
#include <string>

namespace nightwalker::systems {
namespace { constexpr double kMaxAlignmentVerticalDelta = 1.0; }

FeedingController::FeedingController(game::IGameApi& gameApi,game::IGameCombatApi& combatApi,game::IGameFeedingApi& feedingApi,util::Logger& logger,core::Config& config) noexcept
    : gameApi_(gameApi), combatApi_(combatApi), feedingApi_(feedingApi), logger_(logger), config_(config) {}

bool FeedingController::Initialize(){ResetInteraction();resource_.Reset(config_.feeding.initialBlood);logger_.Write(util::LogLevel::Info,"FeedingController initialized; internal resource has no player HUD.");return true;}

bool FeedingController::Request(FeedMode mode,std::uint64_t nowMs) noexcept {
    if(!config_.IsFeatureEnabled(core::Feature::Feeding)||state_!=FeedingState::Idle)return false;
    if(mode==FeedMode::Sip&&!config_.feeding.allowNonLethal)return false;
    mode_=mode;Enter(FeedingState::Candidate,nowMs);return true;
}

void FeedingController::GainHiddenResource(double amount) noexcept {
    if(config_.feeding.hiddenBloodEnabled&&amount>0.0)resource_.Gain(amount);
}

void FeedingController::Update(const core::FrameContext& frame){
    if(state_==FeedingState::Idle)return;
    if(!config_.IsFeatureEnabled(core::Feature::Feeding)){Abort("feeding disabled");return;}
    if(StateTimedOut(frame.nowMs)){Abort("state timeout");return;}
    if(state_==FeedingState::Candidate){if(!AcquireCandidate(frame.nowMs))Abort("no valid aimed feed target");return;}
    if(state_==FeedingState::ReleaseDrain){if(StateElapsed(frame.nowMs)>=static_cast<std::uint64_t>(config_.feeding.releaseMs))Enter(FeedingState::Cleanup,frame.nowMs);return;}
    if(state_==FeedingState::Cleanup){CleanupOwnedTasks();ResetInteraction();return;}
    if(mode_==FeedMode::Drain&&target_!=0&&!gameApi_.PedAlive(target_)&&state_==FeedingState::FeedLoop){ApplyCompletion();Enter(FeedingState::ReleaseDrain,frame.nowMs);return;}
    const char* reason=nullptr;if(!ValidateParticipants(reason)){Abort(reason?reason:"participant validation failed");return;}
    switch(state_){
        case FeedingState::Align:
            if(StateElapsed(frame.nowMs)>=static_cast<std::uint64_t>(config_.feeding.alignMs)){Enter(FeedingState::Grab,frame.nowMs);BeginGrab();}
            break;
        case FeedingState::Grab:
            if(StateElapsed(frame.nowMs)>=static_cast<std::uint64_t>(config_.feeding.grabMs)){
                Enter(FeedingState::FeedLoop,frame.nowMs);
                const int hold=mode_==FeedMode::Sip?config_.feeding.sipDurationMs:config_.feeding.drainDurationMs;
                if(mode_==FeedMode::Sip||!feedingApi_.IsHuman(target_)){feedingApi_.StandStill(player_,hold);feedingApi_.StandStill(target_,hold);playerTaskOwned_=true;targetTaskOwned_=true;}
            }
            break;
        case FeedingState::FeedLoop:{
            const int duration=mode_==FeedMode::Sip?config_.feeding.sipDurationMs:config_.feeding.drainDurationMs;
            if(StateElapsed(frame.nowMs)>=static_cast<std::uint64_t>(duration)){ApplyCompletion();Enter(FeedingState::ReleaseDrain,frame.nowMs);}
            break;
        }
        default:break;
    }
}

void FeedingController::Cancel() noexcept {if(state_!=FeedingState::Idle)Abort("cancelled");}
void FeedingController::Shutdown() noexcept {Cancel();ResetInteraction();}

bool FeedingController::AcquireCandidate(std::uint64_t nowMs) noexcept {
    player_=gameApi_.PlayerPed();if(!gameApi_.PedAlive(player_))return false;
    target_=combatApi_.PlayerAimedPed(player_);
    const char* reason=nullptr;
    if(!ValidateParticipants(reason)){if(config_.debug.enabled&&reason)logger_.Write(util::LogLevel::Debug,std::string("Feed target rejected: ")+reason);target_=0;return false;}
    completionApplied_=false;
    if(config_.debug.enabled)logger_.Write(util::LogLevel::Debug,std::string("Feed candidate accepted mode=")+ModeName(mode_)+" target="+std::to_string(target_));
    Enter(FeedingState::Align,nowMs);BeginAlignment();return true;
}

bool FeedingController::ValidateParticipants(const char*& reason) const noexcept {
    reason=nullptr;
    if(player_==0||!gameApi_.PedAlive(player_)){reason="player invalid/dead";return false;}
    if(target_==0||target_==player_||!gameApi_.PedAlive(target_)){reason="target invalid/dead";return false;}
    const bool human=feedingApi_.IsHuman(target_);
    if(!human&&!config_.feeding.allowAnimalFeeding){reason="non-human target disabled";return false;}
    if(feedingApi_.IsMissionEntity(target_)){reason="mission-owned target";return false;}
    const bool preGrab=state_==FeedingState::Candidate||state_==FeedingState::Align;
    if(preGrab&&(combatApi_.IsPedInMeleeCombat(player_)||combatApi_.IsPedInMeleeCombat(target_)||combatApi_.IsPedInCombatWith(player_,target_)||combatApi_.IsPedInCombatWith(target_,player_))){reason="combat state reserved for later combat-feed hook";return false;}
    if(feedingApi_.IsPedRestricted(player_)){reason="player restricted";return false;}
    if(feedingApi_.IsPedRestricted(target_)){reason="target restricted";return false;}
    if(!WithinDistance(state_==FeedingState::FeedLoop?0.65:0.0)){reason="target out of range";return false;}
    if(!feedingApi_.HasClearLos(player_,target_)){reason="line of sight blocked";return false;}
    if(!feeding_math::VerticalAligned(gameApi_.EntityCoords(player_),gameApi_.EntityCoords(target_),kMaxAlignmentVerticalDelta)){reason="unsafe vertical alignment";return false;}
    return true;
}

bool FeedingController::WithinDistance(double extra) const noexcept {
    if(player_==0||target_==0)return false;
    return feeding_math::WithinRange(gameApi_.EntityCoords(player_),gameApi_.EntityCoords(target_),config_.feeding.maxDistance+std::max(0.0,extra));
}

void FeedingController::Enter(FeedingState state,std::uint64_t nowMs) noexcept {state_=state;stateStartedMs_=nowMs;if(config_.debug.enabled)logger_.Write(util::LogLevel::Debug,std::string("Feeding state -> ")+StateName(state));}

void FeedingController::BeginAlignment() noexcept {
    const bool playerTurn=feedingApi_.FacePedToward(player_,target_);const bool targetTurn=feedingApi_.FacePedToward(target_,player_);
    playerTaskOwned_=playerTaskOwned_||playerTurn;targetTaskOwned_=targetTaskOwned_||targetTurn;
}

void FeedingController::BeginGrab() noexcept {
    if(mode_==FeedMode::Drain&&feedingApi_.IsHuman(target_)){
        if(feedingApi_.StartGrapple(player_,target_)){playerTaskOwned_=true;targetTaskOwned_=true;return;}
        logger_.Write(util::LogLevel::Warning,"Verified grapple task did not start; using stationary drain fallback.");
    }
    const int hold=config_.feeding.grabMs+std::max(config_.feeding.sipDurationMs,config_.feeding.drainDurationMs);
    feedingApi_.StandStill(player_,hold);feedingApi_.StandStill(target_,hold);playerTaskOwned_=true;targetTaskOwned_=true;
}

void FeedingController::ApplyCompletion() noexcept {
    if(completionApplied_)return;completionApplied_=true;
    if(mode_==FeedMode::Drain&&gameApi_.PedAlive(target_))feedingApi_.SetHealth(target_,0);
    const int gain=mode_==FeedMode::Sip?config_.feeding.healthRestoreSip:config_.feeding.healthRestoreDrain;
    const int current=feedingApi_.Health(player_);const int maximum=feedingApi_.MaxHealth(player_);
    if(maximum>0&&current>0&&gain>0)feedingApi_.SetHealth(player_,std::min(maximum,current+gain));
    if(config_.feeding.hiddenBloodEnabled)resource_.Gain(mode_==FeedMode::Sip?config_.feeding.sipBloodGain:config_.feeding.drainBloodGain);
    if(config_.debug.enabled)logger_.Write(util::LogLevel::Debug,std::string("Feed completed mode=")+ModeName(mode_)+" internalResource="+std::to_string(resource_.Value()));
}

void FeedingController::Abort(std::string_view reason) noexcept {if(state_==FeedingState::Idle)return;if(config_.debug.enabled)logger_.Write(util::LogLevel::Debug,std::string("Feed aborted: ")+std::string(reason));CleanupOwnedTasks();ResetInteraction();}
void FeedingController::CleanupOwnedTasks() noexcept {if(playerTaskOwned_&&player_!=0&&gameApi_.EntityExists(player_))feedingApi_.ClearTasks(player_);if(targetTaskOwned_&&target_!=0&&gameApi_.EntityExists(target_))feedingApi_.ClearTasks(target_);playerTaskOwned_=false;targetTaskOwned_=false;}
void FeedingController::ResetInteraction() noexcept {state_=FeedingState::Idle;mode_=FeedMode::Sip;player_=0;target_=0;stateStartedMs_=0;playerTaskOwned_=false;targetTaskOwned_=false;completionApplied_=false;}
bool FeedingController::StateTimedOut(std::uint64_t nowMs) const noexcept {return state_!=FeedingState::Idle&&StateElapsed(nowMs)>static_cast<std::uint64_t>(config_.feeding.stateTimeoutMs);}
std::uint64_t FeedingController::StateElapsed(std::uint64_t nowMs) const noexcept {return nowMs>=stateStartedMs_?nowMs-stateStartedMs_:0;}
const char* FeedingController::StateName(FeedingState state) noexcept {switch(state){case FeedingState::Idle:return "Idle";case FeedingState::Candidate:return "Candidate";case FeedingState::Align:return "Align";case FeedingState::Grab:return "Grab";case FeedingState::FeedLoop:return "FeedLoop";case FeedingState::ReleaseDrain:return "ReleaseDrain";case FeedingState::Cleanup:return "Cleanup";default:return "Unknown";}}
const char* FeedingController::ModeName(FeedMode mode) noexcept {return mode==FeedMode::Sip?"Sip":"Drain";}

} // namespace nightwalker::systems
