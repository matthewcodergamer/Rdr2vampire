#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include "nightwalker/core/Config.h"

namespace nightwalker::core {

inline constexpr int kNightwalkerSaveSchemaVersion = 1;

struct ProgressionUnlocks final {
    bool targetedShadowstep{true};
    bool enhancedFlankLogic{false};
    bool regeneration{false};
};

struct ProgressionTuning final {
    double shadowstepRangeMultiplier{1.0};
    double shadowstepCooldownMultiplier{1.0};
    double feedingEfficiencyMultiplier{1.0};
    double sprintMultiplier{1.0};
    double throwStrengthMultiplier{1.0};
};

struct NightwalkerSaveData final {
    int schemaVersion{kNightwalkerSaveSchemaVersion};
    bool saintDenisCompleted{false};
    std::int64_t saintDenisCooldownUntilGameSeconds{0};
    double hiddenBlood{50.0};
    int progressionPoints{0};
    ProgressionUnlocks unlocks{};
    ProgressionTuning tuning{};
};

enum class SaveLoadStatus {
    Missing,
    Loaded,
    Migrated,
    RecoveredBackup,
    CorruptRecoveredDefaults,
    UnsupportedFutureVersion,
};

struct SaveLoadResult final {
    SaveLoadStatus status{SaveLoadStatus::Missing};
    NightwalkerSaveData data{};
    bool writeAllowed{true};
};

using SaveDiagnosticSink = std::function<void(std::string_view)>;

[[nodiscard]] std::string SerializeSaveData(const NightwalkerSaveData& data);
[[nodiscard]] bool ParseSaveData(std::string_view text, NightwalkerSaveData& data,
                                 SaveDiagnosticSink diagnostics = {});
void NormalizeSaveData(NightwalkerSaveData& data, SaveDiagnosticSink diagnostics = {});
[[nodiscard]] SaveLoadResult LoadSaveData(const std::filesystem::path& path,
                                          SaveDiagnosticSink diagnostics = {});
[[nodiscard]] bool StoreSaveDataAtomicish(const std::filesystem::path& path,
                                          const NightwalkerSaveData& data,
                                          SaveDiagnosticSink diagnostics = {});

// Applies only currently player-owned/runtime-safe tuning. Boss-only systems are not
// silently strengthened by progression. Stored sprint/flank/regeneration fields are
// retained for later explicitly-approved player gameplay consumers.
void ApplyProgressionTuning(Config& config, const NightwalkerSaveData& data) noexcept;

// Deterministic development preset helper for tests/tools. It does not create UI.
void AdvanceDebugProgression(NightwalkerSaveData& data) noexcept;

} // namespace nightwalker::core
