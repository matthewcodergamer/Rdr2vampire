#include "nightwalker/core/DebugInput.h"
#include <Windows.h>
namespace nightwalker::core{
void DebugInput::Configure(const DebugSettings&s)noexcept{enabled_=s.enabled;reloadKey_=s.reloadHotkey;restoreKey_=s.restoreHotkey;wasDown_.fill(false);}
void DebugInput::Reset()noexcept{enabled_=false;wasDown_.fill(false);}
bool DebugInput::PressedOnce(int key)noexcept{if(key<0||key>=256)return false;bool down=(::GetAsyncKeyState(key)&0x8000)!=0;bool pressed=down&&!wasDown_[(std::size_t)key];wasDown_[(std::size_t)key]=down;return pressed;}
DebugActions DebugInput::Poll()noexcept{DebugActions out{};if(!enabled_)return out;out.spawnTestPed=PressedOnce(spawnKey_);out.despawnTestPed=PressedOnce(despawnKey_);out.reloadConfig=PressedOnce(reloadKey_);out.restoreState=PressedOnce(restoreKey_);return out;}
}
