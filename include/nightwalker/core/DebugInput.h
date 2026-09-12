#pragma once
#include <array>
#include "nightwalker/core/Config.h"
namespace nightwalker::core {
struct DebugActions {
 bool reloadConfig{false};
 bool restoreState{false};
 bool spawnTestPed{false};
 bool despawnTestPed{false};
 bool shadowstepForward{false};
 bool feedSip{false};
 bool feedDrain{false};
 bool heavyStrike{false};
 bool grabControl{false};
 bool grabThrow{false};
 bool combatFeed{false};
};
class DebugInput final {
public:
 void Configure(const DebugSettings& settings) noexcept;
 void Reset() noexcept;
 DebugActions Poll() noexcept;
 bool IsEnabled() const noexcept { return enabled_; }
private:
 bool PressedOnce(int key) noexcept;
 bool enabled_{false};
 int heavyStrikeKey_{0x70};   // F1
 int grabControlKey_{0x71};   // F2
 int grabThrowKey_{0x72};     // F3
 int combatFeedKey_{0x73};    // F4
 int feedSipKey_{0x74};       // F5
 int feedDrainKey_{0x75};     // F6
 int shadowstepKey_{0x76};    // F7
 int spawnKey_{0x77};         // F8
 int despawnKey_{0x78};       // F9
 int reloadKey_{0x79};        // F10
 int restoreKey_{0x7A};       // F11
 std::array<bool,256> wasDown_{};
};
}
