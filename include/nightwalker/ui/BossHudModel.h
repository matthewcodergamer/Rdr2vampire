#pragma once

#include <cstdint>

namespace nightwalker::ui {

enum class BossHudState {
    Hidden,
    FadeIn,
    Visible,
    FadeOut,
    DeathHold,
};

struct BossHudTuning final {
    double idleSeconds{6.0};
    double fadeSeconds{0.35};
    double deathHoldSeconds{1.25};
    double healthSmoothingPerSecond{5.0};
};

struct BossHudLayout final {
    float centerX{0.5F};
    float barY{0.875F};
    float titleY{0.837F};
    float barWidth{0.34F};
    float borderHeight{0.020F};
    float backingHeight{0.014F};
    float fillHeight{0.009F};
    float titleScale{0.30F};
};

class BossHudModel final {
public:
    void Reset() noexcept;
    void Begin(float healthRatio, std::uint64_t nowMs) noexcept;
    void Activity(std::uint64_t nowMs) noexcept;
    void SetHealth(float healthRatio) noexcept;
    void EndDefeated(std::uint64_t nowMs) noexcept;
    void Tick(std::uint64_t nowMs, double deltaSeconds, const BossHudTuning& tuning) noexcept;

    [[nodiscard]] BossHudState State() const noexcept { return state_; }
    [[nodiscard]] float ActualHealthRatio() const noexcept { return actualHealthRatio_; }
    [[nodiscard]] float DisplayHealthRatio() const noexcept { return displayHealthRatio_; }
    [[nodiscard]] float Alpha() const noexcept { return alpha_; }
    [[nodiscard]] std::uint64_t LastActivityMs() const noexcept { return lastActivityMs_; }
    [[nodiscard]] bool ShouldDraw() const noexcept { return state_ != BossHudState::Hidden && alpha_ > 0.001F; }

private:
    void SetState(BossHudState next, std::uint64_t nowMs) noexcept;

    BossHudState state_{BossHudState::Hidden};
    float actualHealthRatio_{1.0F};
    float displayHealthRatio_{1.0F};
    float alpha_{0.0F};
    std::uint64_t stateStartedMs_{0};
    std::uint64_t lastActivityMs_{0};
};

[[nodiscard]] float ClampHealthRatio(float value) noexcept;
[[nodiscard]] float SmoothHealthRatio(float current, float target, double deltaSeconds,
                                      double ratePerSecond) noexcept;
[[nodiscard]] BossHudLayout ComputeBossHudLayout(int screenWidth, int screenHeight) noexcept;

} // namespace nightwalker::ui
