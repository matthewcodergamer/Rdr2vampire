#include "nightwalker/core/SafetyWatchdog.h"
#include <utility>
namespace nightwalker::core {
bool SafetyWatchdog::Own(OwnedState s,Action a){if(!a)return false;auto&slot=actions_[Index(s)];if(slot)return false;slot=std::move(a);return true;}
void SafetyWatchdog::Release(OwnedState s)noexcept{actions_[Index(s)]={};}
bool SafetyWatchdog::IsOwned(OwnedState s)const noexcept{return(bool)actions_[Index(s)];}
std::size_t SafetyWatchdog::OwnedCount()const noexcept{std::size_t n=0;for(const auto&a:actions_)if(a)++n;return n;}
bool SafetyWatchdog::Restore(OwnedState s)noexcept{auto&slot=actions_[Index(s)];Action action=std::move(slot);slot={};if(!action)return true;try{action();return true;}catch(...){return false;}}
std::size_t SafetyWatchdog::RestoreAll()noexcept{std::size_t failures=0;for(std::size_t i=0;i<actions_.size();++i)if(!Restore(static_cast<OwnedState>(i)))++failures;return failures;}
}
