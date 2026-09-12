#include "nightwalker/core/Runtime.h"
#include <Windows.h>
#include <array>
#include <filesystem>
#include <string>
#include <utility>
#include "nightwalker/Version.h"

namespace nightwalker::core {
namespace {
bool ModulePath(HMODULE module,std::filesystem::path& out){if(!module)return false;std::array<wchar_t,32768> b{};DWORD n=::GetModuleFileNameW(module,b.data(),(DWORD)b.size());if(!n||n>=b.size())return false;out=std::filesystem::path(std::wstring_view(b.data(),n));return true;}
}

Runtime::Runtime()
    : debugVampireSpawner_(gameApi_, logger_, config_),
      shadowstepController_(gameApi_, gameCombatApi_, logger_, config_),
      vampireAiController_(gameApi_, gameCombatApi_, gamePresentationApi_, debugVampireSpawner_, logger_, config_),
      movementController_(gameApi_, gameCombatApi_, gameMovementApi_, gamePresentationApi_, debugVampireSpawner_, vampireAiController_, logger_, config_),
      feedingController_(gameApi_, gameCombatApi_, gameFeedingApi_, logger_, config_) {}
Runtime::~Runtime()noexcept{Shutdown();}

bool Runtime::Initialize(HMODULE module){
 if(initialized_)return true;
 std::filesystem::path path;if(!ModulePath(module,path)||!gameContext_.Initialize(std::move(path)))return false;
 bool fileLog=logger_.Initialize(gameContext_.PluginDirectory()/L"Nightwalker.log");
 logger_.Write(util::LogLevel::Info,std::string("Starting ")+version::kProjectName+" "+version::kString+" (Story Mode only).");
 if(!fileLog)logger_.Write(util::LogLevel::Warning,"Nightwalker.log unavailable; debugger logging remains active.");
 const auto iniPath=gameContext_.PluginDirectory()/L"Nightwalker.ini";
 config_=Config::Load(iniPath,[this](std::string_view m){logger_.Write(util::LogLevel::Warning,m);});
 logger_.SetMinimumLevel(config_.debug.enabled?util::LogLevel::Debug:util::LogLevel::Info);debugInput_.Configure(config_.debug);
 shadowstepController_.ReloadPresentationSettings(iniPath);
 vampireAiController_.ReloadPresentationSettings(iniPath);
 systems_.clear();systems_.push_back(&debugVampireSpawner_);systems_.push_back(&shadowstepController_);systems_.push_back(&vampireAiController_);systems_.push_back(&movementController_);systems_.push_back(&feedingController_);
 for(auto* system:systems_)if(system&&!system->Initialize()){logger_.Write(util::LogLevel::Error,std::string("System initialization failed: ")+std::string(system->Name()));for(auto* s=systems_.rbegin();s!=systems_.rend();++s)if(*s)(*s)->Shutdown();systems_.clear();gameContext_.Reset();logger_.Shutdown();return false;}
 lastTickMs_=util::MonotonicClock::NowMilliseconds();tickCount_=0;unsafeState_=false;initialized_=true;
 logger_.Write(util::LogLevel::Info,"Phase 7 runtime initialized; cleanup-safe debug feeding and hidden resource are available without a player meter.");return true;
}

void Runtime::Shutdown()noexcept{
 if(!initialized_)return;CancelSystems();RestoreOwnedState("shutdown");
 for(auto it=systems_.rbegin();it!=systems_.rend();++it)if(*it)try{(*it)->Shutdown();}catch(...){logger_.Write(util::LogLevel::Error,"System shutdown failed.");}
 systems_.clear();debugInput_.Reset();initialized_=false;tickCount_=0;lastTickMs_=0;unsafeState_=false;gameContext_.Reset();logger_.Write(util::LogLevel::Info,"Nightwalker shutdown complete.");logger_.Shutdown();
}
}
