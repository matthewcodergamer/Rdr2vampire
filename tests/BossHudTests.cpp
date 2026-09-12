#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "nightwalker/core/Config.h"
#include "nightwalker/ui/BossHudModel.h"

namespace {
void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

bool Near(float a, float b, float epsilon = 0.002F) {
    return std::fabs(a - b) <= epsilon;
}
}

int main() {
    using nightwalker::ui::BossHudModel;
    using nightwalker::ui::BossHudState;
    using nightwalker::ui::BossHudTuning;

    Check(Near(nightwalker::ui::ClampHealthRatio(-1.0F), 0.0F), "health ratio clamps low");
    Check(Near(nightwalker::ui::ClampHealthRatio(2.0F), 1.0F), "health ratio clamps high");

    const auto wide = nightwalker::ui::ComputeBossHudLayout(3440, 1440);
    const auto normal = nightwalker::ui::ComputeBossHudLayout(1920, 1080);
    Check(Near(normal.centerX, 0.5F), "boss bar remains centered");
    Check(normal.barY < 0.95F, "boss bar remains inside lower safe margin");
    Check(wide.barWidth < normal.barWidth, "ultrawide layout keeps cinematic bar from stretching");

    BossHudTuning tuning{};
    BossHudModel model{};
    model.Begin(1.0F, 1000);
    Check(model.State() == BossHudState::Hidden, "begin alone does not pin the bar");
    model.Activity(1000);
    Check(model.State() == BossHudState::FadeIn, "combat activity begins fade-in");
    model.Tick(1350, 0.35, tuning);
    Check(model.State() == BossHudState::Visible, "fade-in reaches visible");
    Check(Near(model.Alpha(), 1.0F), "visible alpha reaches one");

    model.SetHealth(0.50F);
    model.Tick(1450, 0.10, tuning);
    Check(model.DisplayHealthRatio() < 1.0F && model.DisplayHealthRatio() > 0.50F,
          "health display eases toward authoritative ratio");
    Check(Near(model.ActualHealthRatio(), 0.50F), "authoritative health ratio remains exact");

    model.Tick(7101, 0.016, tuning);
    Check(model.State() == BossHudState::FadeOut, "idle combat fades out");
    model.Activity(7150);
    Check(model.State() == BossHudState::FadeIn, "re-engagement reverses fade-out");
    model.Tick(7500, 0.35, tuning);
    Check(model.State() == BossHudState::Visible, "re-engagement returns visible");

    model.EndDefeated(7600);
    Check(model.State() == BossHudState::DeathHold, "boss death enters death hold");
    Check(Near(model.ActualHealthRatio(), 0.0F), "boss death authoritatively reaches zero");
    model.Tick(8900, 1.30, tuning);
    Check(model.State() == BossHudState::FadeOut, "death hold transitions to fade-out");
    model.Tick(9250, 0.35, tuning);
    Check(model.State() == BossHudState::Hidden, "death fade completes hidden");

    model.Begin(0.75F, 10000);
    model.Activity(10000);
    model.Reset();
    Check(model.State() == BossHudState::Hidden && !model.ShouldDraw(), "abort/reset hides immediately");

    const auto config = nightwalker::core::Config::Parse(
        "[BossHUD]\nDisplayName=COUNT ORLOK\nIdleSeconds=8.5\nFadeSeconds=0.6\nDeathHoldSeconds=1.4\nShowNumericHealth=true\n");
    Check(config.bossHud.displayName == "COUNT ORLOK", "boss title parses from config");
    Check(config.bossHud.showNumericHealth, "numeric health is allowed only when explicitly configured");
    Check(std::fabs(config.bossHud.idleSeconds - 8.5) < 0.001, "boss idle seconds parse");

    std::cout << "Boss HUD tests passed\n";
    return 0;
}
