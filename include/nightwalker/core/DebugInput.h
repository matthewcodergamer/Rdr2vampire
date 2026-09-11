#pragma once
#include <array>
#include "nightwalker/core/Config.h"
namespace nightwalker::core {
struct DebugActions { bool reloadConfig{false}; bool restoreState{false}; };
class DebugInput final {
public:
 void Configure(const DebugSettings& settings) noexcept;
 void Reset() noexcept;
 DebugActions Poll() noexcept;
 bool IsEnabled() const noexcept { return enabled_; }
private:
 bool PressedOnce(int key) noexcept;
 bool enabled_{false};
 int reloadKey_{0x79};
 int restoreKey_{0x7A};
 std::array<bool,256> wasDown_{};
};
}
