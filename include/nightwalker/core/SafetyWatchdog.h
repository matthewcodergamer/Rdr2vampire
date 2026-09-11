#pragma once
#include <array>
#include <cstddef>
#include <functional>
namespace nightwalker::core {
enum class OwnedState : std::size_t { Visibility, Collision, Motion, Protection, Input, Camera, Attachment, Task, Count };
class SafetyWatchdog final {
public:
    using Action = std::function<void()>;
    bool Own(OwnedState state, Action restore);
    void Release(OwnedState state) noexcept;
    bool IsOwned(OwnedState state) const noexcept;
    std::size_t OwnedCount() const noexcept;
    void Restore(OwnedState state) noexcept;
    void RestoreAll() noexcept;
private:
    static constexpr std::size_t Index(OwnedState state) noexcept { return static_cast<std::size_t>(state); }
    std::array<Action, Index(OwnedState::Count)> actions_{};
};
}
