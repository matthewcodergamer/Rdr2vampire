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
 int shadowstepKey_{0x76};
 int spawnKey_{0x77};
 int despawnKey_{0x78};
 int reloadKey_{0x79};
 int restoreKey_{0x7A};
 std::array<bool,256> wasDown_{};
};
}
