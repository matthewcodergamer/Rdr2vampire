#include "ConfigInternal.h"

#include <algorithm>
#include <string>

namespace nightwalker::core::config_internal {
namespace {

constexpr int kDebugSpawnKey = 0x77;        // F8
constexpr int kDebugDespawnKey = 0x78;      // F9
constexpr int kDefaultShadowstepKey = 0x76; // F7
constexpr int kDefaultReloadKey = 0x79;     // F10
constexpr int kDefaultRestoreKey = 0x7A;    // F11

template <typename T>
void Clamp(T& value, T low, T high, const char* name, const Config::DiagnosticSink& diagnostics) {
    const T original = value;
    value = std::clamp(value, low, high);
    if (value != original && diagnostics) {
        diagnostics(std::string(name) + " was outside the safe range and was clamped.");
    }
}

bool DebugHotkeysCollide(const DebugSettings& debug) noexcept {
    if (debug.shadowstepHotkey == debug.reloadHotkey ||
        debug.shadowstepHotkey == debug.restoreHotkey ||
        debug.reloadHotkey == debug.restoreHotkey) {
        return true;
    }

    const int reserved[] = {kDebugSpawnKey, kDebugDespawnKey};
    for (const int key : reserved) {
        if (debug.shadowstepHotkey == key ||
            debug.reloadHotkey == key ||
            debug.restoreHotkey == key) {
            return true;
        }
    }
    return false;
}

} // namespace

void Validate(Config& config, const Config::DiagnosticSink& diagnostics) {
    // Preserve the Phase 1/2 ranges for backwards-compatible existing configs.
    Clamp(config.shadowstep.quickDistance, 1.0, 15.0, "Shadowstep.QuickDistance", diagnostics);
    Clamp(config.shadowstep.aimDistance, 1.0, 25.0, "Shadowstep.AimDistance", diagnostics);
    Clamp(config.shadowstep.cooldownMs, 100, 10000, "Shadowstep.CooldownMs", diagnostics);

    // Phase 3 safety settings.
    Clamp(config.shadowstep.maxVerticalDelta, 0.25, 3.0, "Shadowstep.MaxVerticalDelta", diagnostics);
    Clamp(config.shadowstep.validationTimeoutMs, 50, 2000, "Shadowstep.ValidationTimeoutMs", diagnostics);
    Clamp(config.shadowstep.wallClearance, 0.40, 1.50, "Shadowstep.WallClearance", diagnostics);

    // Existing ranges from the runtime foundation.
    Clamp(config.movement.sprintMoveRate, 1.0, 2.0, "Movement.SprintMoveRate", diagnostics);
    Clamp(config.encounter.startHour, 0, 23, "Encounter.StartHour", diagnostics);
    Clamp(config.encounter.endHour, 0, 23, "Encounter.EndHour", diagnostics);
    Clamp(config.encounter.respawnCooldownHours, 1, 720, "Encounter.RespawnCooldownHours", diagnostics);
    Clamp(config.bossHud.idleSeconds, 1.0, 30.0, "BossHUD.IdleSeconds", diagnostics);
    Clamp(config.bossHud.fadeSeconds, 0.1, 3.0, "BossHUD.FadeSeconds", diagnostics);
    Clamp(config.bossHud.deathHoldSeconds, 0.0, 5.0, "BossHUD.DeathHoldSeconds", diagnostics);

    if (DebugHotkeysCollide(config.debug)) {
        if (diagnostics) {
            diagnostics("Debug hotkeys collided with F8/F9 or each other; restoring safe F7/F10/F11 defaults.");
        }
        config.debug.shadowstepHotkey = kDefaultShadowstepKey;
        config.debug.reloadHotkey = kDefaultReloadKey;
        config.debug.restoreHotkey = kDefaultRestoreKey;
    }

    if (config.bossHud.showNumericHealth) {
        if (diagnostics) diagnostics("BossHUD.ShowNumericHealth is locked off by DESIGN_LOCKS.md.");
        config.bossHud.showNumericHealth = false;
    }
}

} // namespace nightwalker::core::config_internal
