#pragma once

#include <chrono>
#include <cstdint>

namespace nightwalker::util {

using SteadyClock = std::chrono::steady_clock;

class MonotonicClock final {
public:
    [[nodiscard]] static std::uint64_t NowMilliseconds() noexcept {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                SteadyClock::now().time_since_epoch()).count());
    }

    [[nodiscard]] static double NowSeconds() noexcept {
        return std::chrono::duration<double>(SteadyClock::now().time_since_epoch()).count();
    }
};

class Deadline final {
public:
    void Clear() noexcept { dueMs_ = 0; }
    void Arm(std::uint64_t nowMs, std::uint64_t delayMs) noexcept { dueMs_ = nowMs + delayMs; }
    [[nodiscard]] bool IsArmed() const noexcept { return dueMs_ != 0; }
    [[nodiscard]] bool HasElapsed(std::uint64_t nowMs) const noexcept {
        return IsArmed() && nowMs >= dueMs_;
    }
    [[nodiscard]] std::uint64_t DueMilliseconds() const noexcept { return dueMs_; }
private:
    std::uint64_t dueMs_{0};
};

}