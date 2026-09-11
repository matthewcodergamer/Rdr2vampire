#pragma once
#include <filesystem>
#include <functional>
#include <string_view>

namespace nightwalker::core {

enum class Feature { Shadowstep, Movement, Feeding, Encounter, BossHud };

struct DebugSettings {
    bool enabled{false};
    int shadowstepHotkey{0x76}; // F7
    int restoreHotkey{0x7A};    // F11
    int reloadHotkey{0x79};     // F10
};

struct ShadowstepSettings {
    bool enabled{true};
    double quickDistance{6.5};
    double aimDistance{9.0};
    int cooldownMs{550};
    double maxVerticalDelta{1.5};
    int validationTimeoutMs{250};
    double wallClearance{0.65};
};

struct MovementSettings { bool enabled{true}; double sprintMoveRate{1.20}; };
struct FeedingSettings { bool enabled{true}; bool allowNonLethal{true}; bool allowAnimalFeeding{true}; };
struct EncounterSettings { bool enabled{true}; int startHour{0}; int endHour{4}; int respawnCooldownHours{24}; };
struct BossHudSettings { bool enabled{true}; double idleSeconds{6.0}; double fadeSeconds{0.35}; double deathHoldSeconds{1.25}; bool showNumericHealth{false}; };

struct Config final {
    using DiagnosticSink = std::function<void(std::string_view)>;
    bool enabled{true};
    DebugSettings debug{};
    ShadowstepSettings shadowstep{};
    MovementSettings movement{};
    FeedingSettings feeding{};
    EncounterSettings encounter{};
    BossHudSettings bossHud{};

    [[nodiscard]] bool IsFeatureEnabled(Feature feature) const noexcept;
    static Config Load(const std::filesystem::path& path, DiagnosticSink diagnostics = {});
    static Config Parse(std::string_view text, DiagnosticSink diagnostics = {});
};

}
