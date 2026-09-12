#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "nightwalker/core/Config.h"
#include "nightwalker/systems/BossActorRegistry.h"
#include "nightwalker/systems/EncounterMath.h"
#include "nightwalker/systems/SaintDenisSettingsLoader.h"

namespace {
int failures = 0;
void Check(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
}

int main() {
    using namespace nightwalker;
    using systems::encounter_math::IsHourInWindow;

    Check(IsHourInWindow(0, 0, 4), "midnight belongs to default window");
    Check(IsHourInWindow(3, 0, 4), "03:00 belongs to default window");
    Check(!IsHourInWindow(4, 0, 4), "end hour is exclusive");
    Check(IsHourInWindow(23, 22, 4), "cross-midnight window includes late night");
    Check(IsHourInWindow(2, 22, 4), "cross-midnight window includes pre-dawn");
    Check(!IsHourInWindow(12, 22, 4), "cross-midnight window excludes daytime");
    Check(IsHourInWindow(12, 5, 5), "equal endpoints mean all-day window");

    const game::Vec3 a{0.0F, 0.0F, 0.0F};
    const game::Vec3 b{3.0F, 4.0F, 99.0F};
    Check(std::fabs(systems::encounter_math::Distance2D(a, b) - 5.0F) < 0.001F,
          "encounter radius uses horizontal distance");
    Check(systems::encounter_math::WithinRadius(a, b, 5.0F), "radius includes exact boundary");
    Check(!systems::encounter_math::WithinRadius(a, b, 4.99F), "radius rejects outside point");

    Check(systems::encounter_math::AddCooldownHours(100, 2) == 7300,
          "hour cooldown converts to game seconds");
    Check(systems::encounter_math::AddCooldownMinutes(100, 10) == 700,
          "abort cooldown converts to game seconds");
    Check(!systems::encounter_math::CooldownExpired(699, 700), "cooldown remains active before boundary");
    Check(systems::encounter_math::CooldownExpired(700, 700), "cooldown expires at boundary");

    systems::BossActorRegistry registry;
    Check(registry.Claim(42, systems::BossOwner::Encounter), "encounter can claim empty registry");
    Check(!registry.Claim(43, systems::BossOwner::Debug), "second actor cannot claim occupied registry");
    Check(!registry.SetCombatEnabled(42, systems::BossOwner::Debug, true),
          "wrong owner cannot arm combat");
    Check(registry.SetCombatEnabled(42, systems::BossOwner::Encounter, true),
          "encounter owner can arm combat");
    Check(registry.CombatEnabled(), "combat armed state is visible");
    Check(!registry.Release(42, systems::BossOwner::Debug), "wrong owner cannot release registry");
    Check(registry.Release(42, systems::BossOwner::Encounter), "correct owner releases registry");
    Check(registry.Ped() == 0 && registry.Owner() == systems::BossOwner::None && !registry.CombatEnabled(),
          "release resets all registry state");

    core::EncounterSettings settings{};
    const std::filesystem::path ini = "Nightwalker.encounter-test.ini";
    {
        std::ofstream out(ini);
        out << "[Encounter.SaintDenis]\n"
               "CenterX=123.5\n"
               "CenterY=-456.25\n"
               "TriggerRadius=5\n"
               "AbortRadius=5\n"
               "SpawnMinDistance=70\n"
               "SpawnMaxDistance=10\n"
               "OmenDurationMs=25000\n"
               "AbortCooldownMinutes=0\n";
    }
    int diagnostics = 0;
    systems::LoadSaintDenisSettings(ini, settings, [&](std::string_view) { ++diagnostics; });
    std::error_code ec;
    std::filesystem::remove(ini, ec);
    Check(std::fabs(settings.centerX - 123.5) < 0.001, "settings loader reads center X");
    Check(std::fabs(settings.centerY + 456.25) < 0.001, "settings loader reads center Y");
    Check(settings.triggerRadius == 20.0, "trigger radius is safely clamped");
    Check(settings.abortRadius >= settings.triggerRadius + 10.0,
          "abort radius remains outside trigger radius");
    Check(settings.spawnMinDistance == 60.0, "spawn minimum is capped");
    Check(settings.spawnMaxDistance >= settings.spawnMinDistance + 5.0,
          "spawn maximum is repaired above minimum");
    Check(settings.omenDurationMs == 10000, "omen duration is capped");
    Check(settings.abortCooldownMinutes == 1, "abort cooldown has safe minimum");
    Check(diagnostics >= 6, "clamped encounter settings emit diagnostics");

    if (failures != 0) {
        std::cerr << failures << " encounter tests failed\n";
        return 1;
    }
    std::cout << "Nightwalker encounter tests passed\n";
    return 0;
}
