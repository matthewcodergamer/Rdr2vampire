#include "nightwalker/core/Runtime.h"

#include <algorithm>

#include "nightwalker/systems/EncounterMath.h"

namespace nightwalker::core {

void Runtime::LoadSaveState(const std::filesystem::path& path) {
    savePath_ = path;
    const auto loaded = LoadSaveData(path, [this](std::string_view message) {
        logger_.Write(util::LogLevel::Warning, message);
    });
    saveData_ = loaded.data;
    saveWriteAllowed_ = loaded.writeAllowed;
    if (loaded.status == SaveLoadStatus::Missing || loaded.status == SaveLoadStatus::CorruptRecoveredDefaults) {
        saveData_.hiddenBlood = std::clamp(config_.feeding.initialBlood, 0.0, 100.0);
    }
    baseEncounterEnabled_ = config_.encounter.enabled;
    ApplySaveToConfig();
    RefreshSavedEncounterGate();
    lastSavedSnapshot_ = SerializeSaveData(saveData_);
    logger_.Write(util::LogLevel::Info,
        loaded.status == SaveLoadStatus::Missing ?
        "Nightwalker save missing; safe defaults active." :
        "Nightwalker save state loaded; no progression HUD is exposed.");
}

void Runtime::ApplySaveToConfig() noexcept {
    ApplyProgressionTuning(config_, saveData_);
    config_.feeding.initialBlood = std::clamp(saveData_.hiddenBlood, 0.0, 100.0);
}

void Runtime::RefreshSavedEncounterGate() noexcept {
    if (saveData_.saintDenisCooldownUntilGameSeconds <= 0) {
        savedEncounterGateActive_ = false;
        config_.encounter.enabled = baseEncounterEnabled_;
        return;
    }
    const auto now = gameEncounterApi_.GameSecondsSinceBaseYear();
    if (!systems::encounter_math::CooldownExpired(now, saveData_.saintDenisCooldownUntilGameSeconds)) {
        savedEncounterGateActive_ = true;
        config_.encounter.enabled = false;
        return;
    }
    savedEncounterGateActive_ = false;
    saveData_.saintDenisCooldownUntilGameSeconds = 0;
    config_.encounter.enabled = baseEncounterEnabled_;
}

} // namespace nightwalker::core
