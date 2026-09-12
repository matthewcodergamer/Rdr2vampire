#include "nightwalker/core/SaveData.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
int failures = 0;
void Check(bool value, const char* name) {
    if (!value) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
}
bool Near(double a, double b, double eps = 0.0001) { return std::fabs(a - b) <= eps; }
}

int main() {
    using namespace nightwalker::core;

    NightwalkerSaveData original{};
    original.saintDenisCompleted = true;
    original.saintDenisCooldownUntilGameSeconds = 123456;
    original.hiddenBlood = 73.5;
    original.progressionPoints = 4;
    original.unlocks.enhancedFlankLogic = true;
    original.tuning.shadowstepRangeMultiplier = 1.10;
    original.tuning.shadowstepCooldownMultiplier = 0.86;
    original.tuning.feedingEfficiencyMultiplier = 1.20;
    original.tuning.sprintMultiplier = 1.03;
    original.tuning.throwStrengthMultiplier = 1.06;

    const std::string serialized = SerializeSaveData(original);
    NightwalkerSaveData parsed{};
    Check(ParseSaveData(serialized, parsed), "schema1 round trip parses");
    Check(parsed.saintDenisCompleted, "completion round trip");
    Check(parsed.saintDenisCooldownUntilGameSeconds == 123456, "cooldown round trip");
    Check(Near(parsed.hiddenBlood, 73.5), "blood round trip");
    Check(parsed.progressionPoints == 4, "points round trip");
    Check(parsed.unlocks.enhancedFlankLogic, "unlock round trip");
    Check(Near(parsed.tuning.throwStrengthMultiplier, 1.06), "tuning round trip");

    const char* legacy =
        "version=0\n"
        "bossCompleted=true\n"
        "cooldownUntil=55\n"
        "hunger=42.5\n"
        "points=3\n";
    NightwalkerSaveData migrated{};
    Check(ParseSaveData(legacy, migrated), "schema0 migration hook parses");
    Check(migrated.schemaVersion == kNightwalkerSaveSchemaVersion, "migration upgrades schema");
    Check(migrated.saintDenisCompleted && migrated.saintDenisCooldownUntilGameSeconds == 55,
          "legacy encounter fields migrate");
    Check(Near(migrated.hiddenBlood, 42.5) && migrated.progressionPoints == 3,
          "legacy resource/progression fields migrate");

    NightwalkerSaveData unsafe{};
    unsafe.hiddenBlood = 500.0;
    unsafe.progressionPoints = -4;
    unsafe.saintDenisCooldownUntilGameSeconds = -9;
    unsafe.tuning.shadowstepRangeMultiplier = 9.0;
    unsafe.tuning.shadowstepCooldownMultiplier = 0.01;
    unsafe.tuning.feedingEfficiencyMultiplier = 8.0;
    unsafe.tuning.sprintMultiplier = 3.0;
    unsafe.tuning.throwStrengthMultiplier = 4.0;
    NormalizeSaveData(unsafe);
    Check(Near(unsafe.hiddenBlood, 100.0), "blood clamp");
    Check(unsafe.progressionPoints == 0, "point clamp");
    Check(unsafe.saintDenisCooldownUntilGameSeconds == 0, "cooldown lower clamp");
    Check(Near(unsafe.tuning.shadowstepRangeMultiplier, 1.25), "range tuning clamp");
    Check(Near(unsafe.tuning.shadowstepCooldownMultiplier, 0.65), "cooldown tuning clamp");
    Check(Near(unsafe.tuning.feedingEfficiencyMultiplier, 1.50), "feeding tuning clamp");
    Check(Near(unsafe.tuning.sprintMultiplier, 1.08), "sprint tuning clamp");
    Check(Near(unsafe.tuning.throwStrengthMultiplier, 1.15), "throw tuning clamp");

    NightwalkerSaveData future{};
    Check(!ParseSaveData("schemaVersion=99\nresource.blood=80\n", future),
          "future schema refuses downgrade parse");

    Config config{};
    config.shadowstep.quickDistance = 6.0;
    config.shadowstep.aimDistance = 8.0;
    config.shadowstep.cooldownMs = 1000;
    config.feeding.sipBloodGain = 20.0;
    config.feeding.drainBloodGain = 40.0;
    config.feeding.healthRestoreSip = 10;
    config.feeding.healthRestoreDrain = 30;
    NightwalkerSaveData tuning{};
    tuning.tuning.shadowstepRangeMultiplier = 1.25;
    tuning.tuning.shadowstepCooldownMultiplier = 0.65;
    tuning.tuning.feedingEfficiencyMultiplier = 1.5;
    ApplyProgressionTuning(config, tuning);
    Check(Near(config.shadowstep.quickDistance, 7.5), "range progression applies");
    Check(Near(config.shadowstep.aimDistance, 10.0), "aim progression applies");
    Check(config.shadowstep.cooldownMs == 650, "cooldown progression applies");
    Check(Near(config.feeding.sipBloodGain, 30.0) && Near(config.feeding.drainBloodGain, 60.0),
          "feeding blood progression applies");
    Check(config.feeding.healthRestoreSip == 15 && config.feeding.healthRestoreDrain == 45,
          "feeding health progression applies");

    NightwalkerSaveData debugPreset{};
    for (int i = 0; i < 20; ++i) AdvanceDebugProgression(debugPreset);
    Check(debugPreset.progressionPoints == 10, "debug preset point cap");
    Check(debugPreset.unlocks.enhancedFlankLogic && debugPreset.unlocks.regeneration,
          "debug preset unlock flags");
    Check(Near(debugPreset.tuning.shadowstepRangeMultiplier, 1.25) &&
          Near(debugPreset.tuning.throwStrengthMultiplier, 1.15),
          "debug preset tuning caps");

    const auto root = std::filesystem::temp_directory_path() / "nightwalker-save-tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    const auto path = root / "Nightwalker.state";
    Check(StoreSaveDataAtomicish(path, original), "first atomic-ish store");
    auto loaded = LoadSaveData(path);
    Check(loaded.status == SaveLoadStatus::Loaded && Near(loaded.data.hiddenBlood, 73.5),
          "stored state reloads");

    NightwalkerSaveData second = original;
    second.hiddenBlood = 61.0;
    Check(StoreSaveDataAtomicish(path, second), "replacement atomic-ish store");
    loaded = LoadSaveData(path);
    Check(Near(loaded.data.hiddenBlood, 61.0), "replacement value reloads");
    Check(!std::filesystem::exists(path.string() + ".tmp") &&
          !std::filesystem::exists(path.string() + ".bak"),
          "successful store leaves no temp or backup");

    {
        std::ofstream bad(path, std::ios::trunc);
        bad << "this is not a valid state file\n";
        std::ofstream backup(path.string() + ".bak", std::ios::trunc);
        backup << serialized;
    }
    loaded = LoadSaveData(path);
    Check(loaded.status == SaveLoadStatus::RecoveredBackup, "corrupt primary recovers backup");
    Check(Near(loaded.data.hiddenBlood, 73.5), "backup recovery value");

    std::filesystem::remove_all(root, ec);
    if (failures) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Nightwalker save/progression tests passed\n";
    return EXIT_SUCCESS;
}
