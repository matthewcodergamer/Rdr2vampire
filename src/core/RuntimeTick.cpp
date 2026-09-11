#include "nightwalker/core/Runtime.h"
#include <algorithm>
#include <string>

namespace nightwalker::core {

void Runtime::Tick(){
 if(!initialized_)return;++tickCount_;auto now=util::MonotonicClock::NowMilliseconds();double dt=std::clamp((double)(now-lastTickMs_)/1000.0,0.0,0.25);lastTickMs_=now;
 auto state=gameContext_.QueryRuntimeState();if(state.Unsafe()){if(!unsafeState_){logger_.Write(util::LogLevel::Info,"Game transition detected; cancelling systems and restoring owned state.");CancelSystems();RestoreOwnedState("game transition");}unsafeState_=true;return;}unsafeState_=false;
 if(!debugDebounce_.IsArmed()||debugDebounce_.HasElapsed(now)){auto a=debugInput_.Poll();if(a.shadowstepForward){shadowstepController_.RequestForward(now);debugDebounce_.Arm(now,250);}else if(a.spawnTestPed){debugVampireSpawner_.RequestSpawn(now);debugDebounce_.Arm(now,250);}else if(a.despawnTestPed){debugVampireSpawner_.RequestDespawn();debugDebounce_.Arm(now,250);}else if(a.reloadConfig){ReloadConfig();debugDebounce_.Arm(now,250);}else if(a.restoreState){logger_.Write(util::LogLevel::Info,"Debug cleanup requested.");CancelSystems();RestoreOwnedState("debug cleanup");debugDebounce_.Arm(now,250);}}
 FrameContext frame{now,dt};for(auto* system:systems_)if(system)system->Update(frame);
}

void Runtime::ReloadConfig(){
 bool wasDebug=config_.debug.enabled;const auto iniPath=gameContext_.PluginDirectory()/L"Nightwalker.ini";config_=Config::Load(iniPath,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);});logger_.SetMinimumLevel(config_.debug.enabled?util::LogLevel::Debug:util::LogLevel::Info);debugInput_.Configure(config_.debug);shadowstepController_.ReloadPresentationSettings(iniPath);if(wasDebug&&!config_.debug.enabled){debugVampireSpawner_.RequestDespawn();shadowstepController_.Cancel();}logger_.Write(util::LogLevel::Info,"Configuration reloaded.");
}

void Runtime::CancelSystems()noexcept{for(auto it=systems_.rbegin();it!=systems_.rend();++it)if(*it)try{(*it)->Cancel();}catch(...){logger_.Write(util::LogLevel::Error,"System cancellation failed.");}}

void Runtime::RestoreOwnedState(std::string_view reason)noexcept{auto owned=watchdog_.OwnedCount();auto failed=watchdog_.RestoreAll();if(failed)logger_.Write(util::LogLevel::Error,std::string("Cleanup failures: ")+std::to_string(failed)+" ("+std::string(reason)+")");else if(owned)logger_.Write(util::LogLevel::Info,std::string("Restored owned state: ")+std::to_string(owned));}

}
