#include "nightwalker/core/SafetyWatchdog.h"
#include <utility>

namespace nightwalker::core {

bool SafetyWatchdog::Own(OwnedState state, Action restore) {
    if (!restore) return false;
    auto& slot = actions_[Index(state)];
    if (slot) return false;
    slot = std::move(restore);
    return true;
}

void SafetyWatchdog::Release(OwnedState state) noexcept {
    actions_[Index(state)] = {};
}

bool SafetyWatchdog::IsOwned(OwnedState state) const noexcept {
    return static_cast<bool>(actions_[Index(state)]);
}

std::size_t SafetyWatchdog::OwnedCount() const noexcept {
    std::size_t count = 0;
    for (const auto& action : actions_) if (action) ++count;
    return count;
}

void SafetyWatchdog::Restore(OwnedState state) noexcept {
    auto& slot = actions_[Index(state)];
    Action action = std::move(slot);
    slot = {};
    if (!action) return;
    try { action(); } catch (...) { }
}

void SafetyWatchdog::RestoreAll() noexcept {
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        Restore(static_cast<OwnedState>(i));
    }
}

}
