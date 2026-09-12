#include "nightwalker/core/Runtime.h"
#include <algorithm>
#include <chrono>
#include <string>
#include "nightwalker/narrative/NarrativeSettingsLoader.h"
#include "nightwalker/systems/SaintDenisSettingsLoader.h"
namespace nightwalker::core{
namespace{
constexpr std::uint64_t kBossValidationIntervalMs=250;
constexpr std::uint64_t kProfileReportIntervalMs=10000;
}
void Runtime::Tick(){
 if(!initialized_)return;++tickCount_;auto now=util::MonotonicClock::NowMilliseconds();double dt=std::clamp((double)(now-lastTickMs_)/1000.0,0.0,0.25);lastTickMs_=now;
 const auto state=gameContext_.QueryRuntimeState();RuntimeObservation observation{};observation.nowMs=now;observation.storySafe=!state.Unsafe();
 if(observation.storySafe){observation.playerPed=gameApi_.PlayerPed();if(observation.playerPed==0||!gameApi_.EntityExists(observation.playerPed))observation.storySafe=false;else{observation.playerPosition=gameApi_.EntityCoords(observation.playerPed);observation.positionKnown=true;}}
 const auto recovery=sessionGuard_.Observe(observation);if(recovery.cleanupRequested){const char* reason=LongSessionGuard::TriggerName(recovery.trigger);logger_.Write(util::LogLevel::Info,std::string("Long-session recovery triggered: ")+reason+".");CancelSystems();RestoreOwnedState(reason);}if(recovery.suspendUpdates)return;
 if(!ValidateBossReference(now))return;
 if(!debugDebounce_.IsArmed()||debugDebounce_.HasElapsed(now)){auto a=debugInput_.Poll();const auto player=gameApi_.PlayerPed();
  if(a.heavyStrike){feedingController_.Cancel();shadowstepController_.Cancel();vampireCombatController_.CancelForActor(player);vampireCombatController_.RequestPlayerDebug(systems::CombatMove::HeavyStrike,now);debugDebounce_.Arm(now,250);}
  else if(a.grabControl){feedingController_.Cancel();shadowstepController_.Cancel();vampireCombatController_.CancelForActor(player);vampireCombatController_.RequestPlayerDebug(systems::CombatMove::GrabControl,now);debugDebounce_.Arm(now,250);}
  else if(a.grabThrow){if(!vampireCombatController_.RequestGrabFollowup(systems::CombatMove::GrabThrow)){feedingController_.Cancel();shadowstepController_.Cancel();vampireCombatController_.CancelForActor(player);vampireCombatController_.RequestPlayerDebug(systems::CombatMove::GrabThrow,now);}debugDebounce_.Arm(now,250);}
  else if(a.combatFeed){if(!vampireCombatController_.RequestGrabFollowup(systems::CombatMove::CombatFeed)){feedingController_.Cancel();shadowstepController_.Cancel();vampireCombatController_.CancelForActor(player);vampireCombatController_.RequestPlayerDebug(systems::CombatMove::CombatFeed,now);}debugDebounce_.Arm(now,250);}
  else if(a.feedSip){vampireCombatController_.CancelForActor(player);if(feedingController_.IsActive())feedingController_.Cancel();else{shadowstepController_.Cancel();feedingController_.Request(systems::FeedMode::Sip,now);}debugDebounce_.Arm(now,250);}
  else if(a.feedDrain){vampireCombatController_.CancelForActor(player);if(feedingController_.IsActive())feedingController_.Cancel();else{shadowstepController_.Cancel();feedingController_.Request(systems::FeedMode::Drain,now);}debugDebounce_.Arm(now,250);}
  else if(a.shadowstepForward){if(!feedingController_.IsActive()&&!vampireCombatController_.IsActiveFor(player))shadowstepController_.RequestForward(now);debugDebounce_.Arm(now,250);}
  else if(a.spawnTestPed){debugVampireSpawner_.RequestSpawn(now);debugDebounce_.Arm(now,250);}
  else if(a.despawnTestPed){if(debugVampireSpawner_.Owner()==systems::BossOwner::Debug){vampireCombatController_.Cancel();feedingController_.Cancel();movementController_.Cancel();vampireAiController_.Cancel();debugVampireSpawner_.RequestDespawn();}else if(debugVampireSpawner_.Owner()==systems::BossOwner::Encounter){logger_.Write(util::LogLevel::Debug,"F9 debug despawn ignored while the Saint Denis encounter owns the boss.");}debugDebounce_.Arm(now,250);}
  else if(a.reloadConfig){ReloadConfig();debugDebounce_.Arm(now,250);}
  else if(a.restoreState){logger_.Write(util::LogLevel::Info,"Debug cleanup requested.");CancelSystems();RestoreOwnedState("debug cleanup");sessionGuard_.Reset();debugDebounce_.Arm(now,250);}}
 FrameContext frame{now,dt};UpdateSystems(frame);
}
void Runtime::ReloadConfig(){
 bossHudController_.Cancel();vampireCombatController_.Cancel();movementController_.Cancel();vampireAiController_.Cancel();saintDenisDirector_.Cancel();narrativeController_.Cancel();feedingController_.Cancel();shadowstepController_.Cancel();progressionController_.Cancel();
 bool wasDebug=config_.debug.enabled;const auto iniPath=gameContext_.PluginDirectory()/L"Nightwalker.ini";config_=Config::Load(iniPath,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);});systems::LoadSaintDenisSettings(iniPath,config_.encounter,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);});narrative::LoadNarrativeSettings(iniPath,config_.narrative,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);});progressionController_.ApplyAfterConfigReload();logger_.SetMinimumLevel(config_.debug.enabled?util::LogLevel::Debug:util::LogLevel::Info);debugInput_.Configure(config_.debug);shadowstepController_.ReloadPresentationSettings(iniPath);vampireAiController_.ReloadPresentationSettings(iniPath);narrativeController_.LoadScript(gameContext_.PluginDirectory()/L"Nightwalker.dialogue");if(wasDebug&&!config_.debug.enabled)debugVampireSpawner_.RequestDespawn();sessionGuard_.Reset();nextBossValidationMs_=0;ResetPerformanceProfile();logger_.Write(util::LogLevel::Info,"Configuration reloaded after safe cleanup; Phase 13 runtime guards reset.");
}
bool Runtime::ValidateBossReference(std::uint64_t nowMs)noexcept{
 if(nowMs<nextBossValidationMs_)return true;nextBossValidationMs_=nowMs+kBossValidationIntervalMs;const auto boss=bossRegistry_.Ped();if(boss==0)return true;
 if(gameApi_.EntityExists(boss)&&gameApi_.EntityModel(boss)==systems::kSaintDenisVampireModel)return true;
 logger_.Write(util::LogLevel::Warning,"Boss registry reference became invalid or was reused; cancelling Nightwalker ownership.");CancelSystems();RestoreOwnedState("invalid boss registry reference");bossRegistry_.ForceClear();return false;
}
void Runtime::UpdateSystems(const FrameContext& frame){
 const bool profile=config_.debug.enabled&&config_.debug.profileRuntime;
 for(std::size_t i=0;i<systems_.size();++i){auto* system=systems_[i];if(!system)continue;if(!profile||i>=systemProfiles_.size()){system->Update(frame);continue;}const auto start=std::chrono::steady_clock::now();system->Update(frame);const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();auto& sample=systemProfiles_[i];++sample.updates;sample.totalMicros+=static_cast<std::uint64_t>(std::max<std::int64_t>(0,elapsed));sample.maxMicros=std::max(sample.maxMicros,static_cast<std::uint64_t>(std::max<std::int64_t>(0,elapsed)));}
 if(profile)ReportPerformance(frame.nowMs);
}
void Runtime::ResetPerformanceProfile()noexcept{for(auto& sample:systemProfiles_)sample={};nextProfileReportMs_=0;}
void Runtime::ReportPerformance(std::uint64_t nowMs){
 if(nextProfileReportMs_==0){nextProfileReportMs_=nowMs+kProfileReportIntervalMs;return;}if(nowMs<nextProfileReportMs_)return;
 for(std::size_t i=0;i<systems_.size()&&i<systemProfiles_.size();++i){const auto& sample=systemProfiles_[i];if(sample.updates==0||systems_[i]==nullptr)continue;const auto average=sample.totalMicros/sample.updates;logger_.Write(util::LogLevel::Debug,std::string("Runtime profile ")+std::string(systems_[i]->Name())+" updates="+std::to_string(sample.updates)+" avgUs="+std::to_string(average)+" maxUs="+std::to_string(sample.maxMicros));}
 for(auto& sample:systemProfiles_)sample={};nextProfileReportMs_=nowMs+kProfileReportIntervalMs;
}
void Runtime::CancelSystems()noexcept{for(auto it=systems_.rbegin();it!=systems_.rend();++it)if(*it)try{(*it)->Cancel();}catch(...){logger_.Write(util::LogLevel::Error,std::string("System cancellation failed: ")+std::string((*it)->Name()));}}
void Runtime::RestoreOwnedState(std::string_view reason)noexcept{auto owned=watchdog_.OwnedCount();auto failed=watchdog_.RestoreAll();if(failed)logger_.Write(util::LogLevel::Error,std::string("Cleanup failures: ")+std::to_string(failed)+" ("+std::string(reason)+")");else if(owned)logger_.Write(util::LogLevel::Info,std::string("Restored owned state: ")+std::to_string(owned));}
}
