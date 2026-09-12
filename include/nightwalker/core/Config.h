#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

namespace nightwalker::core {

enum class Feature { Shadowstep, Movement, Feeding, Combat, Encounter, VampireAi, Narrative, BossHud };

struct DebugSettings {
    bool enabled{false};
    int shadowstepHotkey{0x76};
    int restoreHotkey{0x7A};
    int reloadHotkey{0x79};
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

struct MovementSettings {
    bool enabled{true};
    double sprintMoveRate{1.15};
    int accelerationMs{350};
    int burstDurationMs{1800};
    int recoveryMs{900};
    int dismountRecoveryMs{500};
    double activationDistance{4.5};
    double minVelocity{0.30};
    bool trailFx{true};
    int trailIntervalMs{180};
};

struct FeedingSettings {
    bool enabled{true};
    bool allowNonLethal{true};
    bool allowAnimalFeeding{false};
    bool hiddenBloodEnabled{true};
    double initialBlood{50.0};
    double sipBloodGain{20.0};
    double drainBloodGain{55.0};
    int healthRestoreSip{15};
    int healthRestoreDrain{45};
    double maxDistance{1.70};
    int alignMs{350};
    int grabMs{450};
    int sipDurationMs{1200};
    int drainDurationMs{1800};
    int releaseMs{250};
    int stateTimeoutMs{3500};
};

struct CombatSettings {
    bool enabled{true};
    double maxDistance{2.25};
    int heavyWindupMs{260};
    int shadowstepFollowupWindupMs{120};
    int strikeWindowMs{700};
    int recoveryMs{600};
    int grabAlignMs{300};
    int grabHoldMs{400};
    int feedHoldMs{500};
    int throwRagdollMs{1200};
    int stateTimeoutMs{4500};
    int strikeBonus{8};
    int combatFeedCost{18};
    int combatFeedRestore{10};
    double combatFeedBloodGain{12.0};
    double strikeMoveRate{1.08};
    double throwHorizontalForce{1.35};
    double throwUpForce{0.28};
    double throwProjectionMeters{2.25};
    int bossSpecialCooldownMs{2800};
};

struct EncounterSettings {
    bool enabled{true};
    int startHour{0};
    int endHour{4};
    int respawnCooldownHours{24};
    double centerX{2741.01245};
    double centerY{-1263.93384};
    double centerZ{50.61435};
    double triggerRadius{70.0};
    double abortRadius{115.0};
    double spawnMinDistance{18.0};
    double spawnMaxDistance{55.0};
    double confrontationDistance{18.0};
    int eligibilityPollMs{500};
    int omenDurationMs{2600};
    int omenPulseMs{850};
    int spawnRetryMs{450};
    int spawnTimeoutMs{9000};
    int stalkingMs{5000};
    int confrontationMs{900};
    int leaveGraceMs{7000};
    int resolutionHoldMs{1500};
    int abortCooldownMinutes{10};
};

struct VampireAiSettings {
    bool enabled{true};
    double shadowstepMinDistance{4.0};
    double shadowstepMaxDistance{10.0};
    double strikingRange{1.65};
    int predictionMs{250};
    int decisionIntervalMs{180};
    int shadowstepCooldownMs{2400};
    int telegraphMs{320};
    int recoveryMs{850};
    int evadeCooldownMs{5000};
    double retreatSpeedThreshold{0.55};
};

struct NarrativeSettings {
    bool enabled{true};
    int maxConfrontationHoldMs{6000};
    int maxSequenceMs{9000};
    int skipKey{0x0D}; // Enter; active only while a narrative sequence is playing.
    bool optionalAudio{true};
};

struct BossHudSettings {
    bool enabled{true};
    std::string displayName{"THE VAMPIRE"};
    double idleSeconds{6.0};
    double fadeSeconds{0.35};
    double deathHoldSeconds{1.25};
    bool showNumericHealth{false};
};

struct Config final {
    using DiagnosticSink = std::function<void(std::string_view)>;
    bool enabled{true};
    DebugSettings debug{};
    ShadowstepSettings shadowstep{};
    MovementSettings movement{};
    FeedingSettings feeding{};
    CombatSettings combat{};
    EncounterSettings encounter{};
    VampireAiSettings vampireAi{};
    NarrativeSettings narrative{};
    BossHudSettings bossHud{};

    [[nodiscard]] bool IsFeatureEnabled(Feature feature) const noexcept;
    static Config Load(const std::filesystem::path& path, DiagnosticSink diagnostics = {});
    static Config Parse(std::string_view text, DiagnosticSink diagnostics = {});
};

}
