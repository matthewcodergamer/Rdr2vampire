#include "nightwalker/ui/BossHudModel.h"

#include <algorithm>
#include <cmath>

namespace nightwalker::ui {

float ClampHealthRatio(float value) noexcept {
    return std::clamp(value, 0.0F, 1.0F);
}

float SmoothHealthRatio(float current, float target, double deltaSeconds,
                        double ratePerSecond) noexcept {
    current = ClampHealthRatio(current);
    target = ClampHealthRatio(target);
    if (deltaSeconds <= 0.0 || ratePerSecond <= 0.0) return current;
    const double t = std::clamp(deltaSeconds * ratePerSecond, 0.0, 1.0);
    const float value = current + (target - current) * static_cast<float>(t);
    return std::fabs(value - target) < 0.001F ? target : ClampHealthRatio(value);
}

BossHudLayout ComputeBossHudLayout(int screenWidth, int screenHeight) noexcept {
    if (screenWidth <= 0 || screenHeight <= 0) {
        screenWidth = 1920;
        screenHeight = 1080;
    }
    constexpr float kReferenceAspect = 16.0F / 9.0F;
    const float aspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    const float aspectScale = kReferenceAspect / std::max(1.0F, aspect);

    BossHudLayout layout{};
    layout.barWidth = std::clamp(0.34F * aspectScale, 0.22F, 0.36F);
    layout.centerX = 0.5F;
    layout.barY = 0.875F;
    layout.titleY = 0.837F;
    return layout;
}

void BossHudModel::Reset() noexcept {
    state_ = BossHudState::Hidden;
    actualHealthRatio_ = 1.0F;
    displayHealthRatio_ = 1.0F;
    alpha_ = 0.0F;
    stateStartedMs_ = 0;
    lastActivityMs_ = 0;
}

void BossHudModel::Begin(float healthRatio, std::uint64_t nowMs) noexcept {
    actualHealthRatio_ = ClampHealthRatio(healthRatio);
    displayHealthRatio_ = actualHealthRatio_;
    alpha_ = 0.0F;
    lastActivityMs_ = nowMs;
    SetState(BossHudState::Hidden, nowMs);
}

void BossHudModel::Activity(std::uint64_t nowMs) noexcept {
    if (state_ == BossHudState::DeathHold) return;
    lastActivityMs_ = nowMs;
    if (state_ == BossHudState::Hidden || state_ == BossHudState::FadeOut) {
        SetState(BossHudState::FadeIn, nowMs);
    }
}

void BossHudModel::SetHealth(float healthRatio) noexcept {
    actualHealthRatio_ = ClampHealthRatio(healthRatio);
}

void BossHudModel::EndDefeated(std::uint64_t nowMs) noexcept {
    actualHealthRatio_ = 0.0F;
    if (state_ == BossHudState::Hidden) {
        displayHealthRatio_ = 0.0F;
        return;
    }
    alpha_ = 1.0F;
    SetState(BossHudState::DeathHold, nowMs);
}

void BossHudModel::Tick(std::uint64_t nowMs, double deltaSeconds,
                        const BossHudTuning& tuning) noexcept {
    displayHealthRatio_ = SmoothHealthRatio(
        displayHealthRatio_, actualHealthRatio_, deltaSeconds, tuning.healthSmoothingPerSecond);

    const double fadeSeconds = std::max(0.05, tuning.fadeSeconds);
    const float fadeDelta = static_cast<float>(std::max(0.0, deltaSeconds) / fadeSeconds);

    switch (state_) {
        case BossHudState::Hidden:
            alpha_ = 0.0F;
            return;

        case BossHudState::FadeIn:
            alpha_ = std::clamp(alpha_ + fadeDelta, 0.0F, 1.0F);
            if (alpha_ >= 0.999F) {
                alpha_ = 1.0F;
                SetState(BossHudState::Visible, nowMs);
            }
            return;

        case BossHudState::Visible: {
            alpha_ = 1.0F;
            const auto idleMs = static_cast<std::uint64_t>(std::max(0.0, tuning.idleSeconds) * 1000.0);
            if (lastActivityMs_ != 0 && nowMs >= lastActivityMs_ &&
                nowMs - lastActivityMs_ >= idleMs) {
                SetState(BossHudState::FadeOut, nowMs);
            }
            return;
        }

        case BossHudState::FadeOut:
            alpha_ = std::clamp(alpha_ - fadeDelta, 0.0F, 1.0F);
            if (alpha_ <= 0.001F) {
                alpha_ = 0.0F;
                SetState(BossHudState::Hidden, nowMs);
            }
            return;

        case BossHudState::DeathHold: {
            alpha_ = 1.0F;
            const auto holdMs = static_cast<std::uint64_t>(std::max(0.0, tuning.deathHoldSeconds) * 1000.0);
            if (nowMs >= stateStartedMs_ && nowMs - stateStartedMs_ >= holdMs) {
                displayHealthRatio_ = 0.0F;
                SetState(BossHudState::FadeOut, nowMs);
            }
            return;
        }
    }
}

void BossHudModel::SetState(BossHudState next, std::uint64_t nowMs) noexcept {
    state_ = next;
    stateStartedMs_ = nowMs;
}

} // namespace nightwalker::ui
