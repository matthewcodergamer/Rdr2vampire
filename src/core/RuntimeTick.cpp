#include "nightwalker/core/Runtime.h"
#include <algorithm>
#include <string>
#include "nightwalker/narrative/NarrativeSettingsLoader.h"
#include "nightwalker/systems/SaintDenisSettingsLoader.h"
namespace nightwalker::core{
void Runtime::Tick(){
 if(!initialized_)return;++tickCount_;auto now=util::MonotonicClock::NowMilliseconds();double dt=std::clamp((double)(now-lastTickMs_)/1000.0,0.0,0.25);lastTickMs_=now;
 auto state=gameContext_.QueryRuntimeState();if(state.Unsafe()){if(!unsafeState_){logger_.Write(util::LogLevel::Info,"Game transition detected; cancelling systems and restoring owned state.");CancelSystems();RestoreOwnedState("game transition");}unsafeState_=true;return;}unsafeState_=false;
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
  else if(a.restoreState){logger_.Write(util::LogLevel::Info,"Debug cleanup requested.");CancelSystems();RestoreOwnedState("debug cleanup");debugDebounce_.Arm(now,250);}}
 FrameContext frame{now,dt};for(auto* system:systems_)if(system)system->Update(frame);
}
void Runtime::ReloadConfig(){
 bossHudController_.Cancel();vampireCombatController_.Cancel();movementController_.Cancel();vampireAiController_.Cancel();saintDenisDirector_.Cancel();narrativeController_.Cancel();feedingController_.Cancel();shadowstepController_.Cancel();progressionController_.Cancel();
 bool wasDebug=config_.debug.enabled;const auto iniPath=gameContext_.PluginDirectory()/L"Nightwalker.ini";config_=Config::Load(iniPath,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);});systems::LoadSaintDenisSettings(iniPath,config_.encounter,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);});narrative::LoadNarrativeSettings(iniPath,config_.narrative,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);});progressionController_.ApplyAfterConfigReload();logger_.SetMinimumLevel(config_.debug.enabled?util::LogLevel::Debug:util::LogLevel::Info);debugInput_.Configure(config_.debug);shadowstepController_.ReloadPresentationSettings(iniPath);vampireAiController_.ReloadPresentationSettings(iniPath);narrativeController_.LoadScript(gameContext_.PluginDirectory()/L"Nightwalker.dialogue");if(wasDebug&&!config_.debug.enabled)debugVampireSpawner_.RequestDespawn();logger_.Write(util::LogLevel::Info,"Configuration reloaded; progression and narrative data reapplied after safe cleanup.");
}
void Runtime::CancelSystems()noexcept{for(auto it=systems_.rbegin();it!=systems_.rend();++it)if(*it)try{(*it)->Cancel();}catch(...){logger_.Write(util::LogLevel::Error,"System cancellation failed.");}}
void Runtime::RestoreOwnedState(std::string_view reason)noexcept{auto owned=watchdog_.OwnedCount();auto failed=watchdog_.RestoreAll();if(failed)logger_.Write(util::LogLevel::Error,std::string("Cleanup failures: ")+std::to_string(failed)+" ("+std::string(reason)+")");else if(owned)logger_.Write(util::LogLevel::Info,std::string("Restored owned state: ")+std::to_string(owned));}
}
