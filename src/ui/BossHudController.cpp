#include "nightwalker/ui/BossHudController.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace nightwalker::ui {
namespace {
constexpr float kCombatRefreshDistance = 20.0F;

float Distance2D(const game::Vec3& a, const game::Vec3& b) noexcept {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

int AlphaByte(float alpha, int base) noexcept {
    return std::clamp(static_cast<int>(static_cast<float>(base) * std::clamp(alpha, 0.0F, 1.0F)), 0, 255);
}
}

BossHudController::BossHudController(game::IGameApi& api, game::IGameCombatApi& combatApi,
    game::IGameFeedingApi& feedingApi, game::IGamePhysicalApi& physicalApi,
    game::IGameBossBarApi& drawApi, util::Logger& logger,
    const core::Config& config) noexcept
    : api_(api), combatApi_(combatApi), feedingApi_(feedingApi), physicalApi_(physicalApi),
      drawApi_(drawApi), logger_(logger), config_(config) {}

bool BossHudController::Initialize() {
    ForceHide();
    return true;
}

bool BossHudController::BeginBoss(game::PedHandle boss, std::string_view displayName,
                                  std::uint64_t nowMs) noexcept {
    if (!config_.IsFeatureEnabled(core::Feature::BossHud)) {
        ForceHide();
        return false;
    }
    if (boss == 0 || !api_.EntityExists(boss) || !api_.PedAlive(boss)) {
        logger_.Write(util::LogLevel::Warning, "Boss HUD refused an invalid encounter boss handle.");
        ForceHide();
        return false;
    }

    boss_ = boss;
    displayName_ = displayName.empty() ? "THE VAMPIRE" : std::string(displayName);
    const float ratio = ReadBossHealthRatio(bossHealth_, bossMaxHealth_);
    const game::PedHandle player = api_.PlayerPed();
    playerHealth_ = player != 0 && api_.PedAlive(player) ? feedingApi_.Health(player) : 0;
    model_.Begin(ratio, nowMs);
    return true;
}

void BossHudController::NotifyCombatActivity(std::uint64_t nowMs) noexcept {
    if (boss_ == 0 || model_.State() == BossHudState::DeathHold) return;
    model_.Activity(nowMs);
}

void BossHudController::EndBoss(bool defeated, std::uint64_t nowMs) noexcept {
    if (!defeated) {
        ForceHide();
        return;
    }
    if (model_.State() == BossHudState::Hidden) {
        ForceHide();
        return;
    }
    boss_ = 0;
    bossHealth_ = 0;
    model_.EndDefeated(nowMs);
}

void BossHudController::Update(const core::FrameContext& frame) {
    if (!config_.IsFeatureEnabled(core::Feature::BossHud)) {
        ForceHide();
        return;
    }

    if (boss_ != 0) {
        if (!api_.EntityExists(boss_)) {
            ForceHide();
            return;
        }
        if (!api_.PedAlive(boss_)) {
            EndBoss(true, frame.nowMs);
        } else {
            const int previousBossHealth = bossHealth_;
            const float ratio = ReadBossHealthRatio(bossHealth_, bossMaxHealth_);
            model_.SetHealth(ratio);
            if (bossHealth_ < previousBossHealth) NotifyCombatActivity(frame.nowMs);

            const game::PedHandle player = api_.PlayerPed();
            if (player == 0 || !api_.PedAlive(player)) {
                ForceHide();
                return;
            }
            const int currentPlayerHealth = feedingApi_.Health(player);
            if (currentPlayerHealth < playerHealth_ && physicalApi_.WasContactFrom(player, boss_)) {
                NotifyCombatActivity(frame.nowMs);
            }
            playerHealth_ = currentPlayerHealth;

            if (CloseCombatEngagement(player)) NotifyCombatActivity(frame.nowMs);
        }
    }

    BossHudTuning tuning{};
    tuning.idleSeconds = config_.bossHud.idleSeconds;
    tuning.fadeSeconds = config_.bossHud.fadeSeconds;
    tuning.deathHoldSeconds = config_.bossHud.deathHoldSeconds;
    model_.Tick(frame.nowMs, frame.deltaSeconds, tuning);
    Draw();
}

float BossHudController::ReadBossHealthRatio(int& health, int& maxHealth) const noexcept {
    if (boss_ == 0) {
        health = 0;
        maxHealth = std::max(1, maxHealth);
        return 0.0F;
    }
    health = std::max(0, feedingApi_.Health(boss_));
    maxHealth = std::max(1, feedingApi_.MaxHealth(boss_));
    return ClampHealthRatio(static_cast<float>(health) / static_cast<float>(maxHealth));
}

bool BossHudController::CloseCombatEngagement(game::PedHandle player) const noexcept {
    if (boss_ == 0 || player == 0) return false;
    if (!combatApi_.IsPedInCombatWith(boss_, player) && !combatApi_.IsPedInCombatWith(player, boss_)) {
        return false;
    }
    return Distance2D(api_.EntityCoords(boss_), api_.EntityCoords(player)) <= kCombatRefreshDistance;
}

void BossHudController::Draw() noexcept {
    if (!model_.ShouldDraw()) return;

    int width = 1920;
    int height = 1080;
    drawApi_.Resolution(width, height);
    const BossHudLayout layout = ComputeBossHudLayout(width, height);
    const float alpha = model_.Alpha();

    drawApi_.Rectangle(layout.centerX, layout.barY, layout.barWidth + 0.012F,
                       layout.borderHeight, 12, 8, 8, AlphaByte(alpha, 220));
    drawApi_.Rectangle(layout.centerX, layout.barY, layout.barWidth + 0.006F,
                       layout.backingHeight, 28, 24, 23, AlphaByte(alpha, 205));

    const float fillWidth = layout.barWidth * model_.DisplayHealthRatio();
    if (fillWidth > 0.0005F) {
        const float left = layout.centerX - layout.barWidth * 0.5F;
        const float fillCenter = left + fillWidth * 0.5F;
        drawApi_.Rectangle(fillCenter, layout.barY, fillWidth, layout.fillHeight,
                           112, 20, 24, AlphaByte(alpha, 245));
    }

    std::string title = displayName_;
    if (config_.bossHud.showNumericHealth) {
        title += "  ";
        title += std::to_string(std::max(0, bossHealth_));
        title += "/";
        title += std::to_string(std::max(1, bossMaxHealth_));
    }
    drawApi_.CenteredText(title.c_str(), layout.centerX, layout.titleY, layout.titleScale,
                          218, 209, 194, AlphaByte(alpha, 235));
}

void BossHudController::ForceHide() noexcept {
    boss_ = 0;
    bossHealth_ = 0;
    bossMaxHealth_ = 1;
    playerHealth_ = 0;
    displayName_ = "THE VAMPIRE";
    model_.Reset();
}

void BossHudController::Cancel() noexcept {
    ForceHide();
}

void BossHudController::Shutdown() noexcept {
    ForceHide();
}

} // namespace nightwalker::ui
