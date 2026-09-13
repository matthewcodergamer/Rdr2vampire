#pragma once
#include <cstdint>
namespace nightwalker::narrative {
enum class ReactiveDialogueEvent { None, Question, Challenge, Leave, AimStarted, AimHeld, AimLowered, ShotStarted, ShotHit, ShotMiss };
struct ReactiveDialogueInput final { std::uint64_t nowMs{0}; bool aimed{false}; bool shooting{false}; bool hitBoss{false}; bool question{false}; bool challenge{false}; bool leave{false}; };
class ReactiveDialogueModel final {
public:
 ReactiveDialogueEvent Update(const ReactiveDialogueInput& input) noexcept;
 void Reset() noexcept;
 [[nodiscard]] bool ShotPending() const noexcept { return shotPending_; }
private:
 bool aimed_{false}; bool shooting_{false}; bool aimHeldEmitted_{false}; bool shotPending_{false}; std::uint64_t aimStartedMs_{0}; std::uint64_t shotDeadlineMs_{0};
};
}
