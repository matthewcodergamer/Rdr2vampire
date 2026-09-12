#include "nightwalker/systems/ProgressionController.h"

#include <algorithm>

#include "nightwalker/systems/EncounterMath.h"

namespace nightwalker::systems {

ProgressionController::ProgressionController(core::Config& config,
                                             game::IGameEncounterApi& encounterApi,
                                             FeedingController& feeding,
                                             SaintDenisDirector& encounter,
                                             util::Logger& logger) noexcept
    : config_(config), encounterApi_(encounterApi), feeding_(feeding),
      encounter_(encounter), logger_(logger) {}

bool ProgressionController::Initialize() {
    nextCheckMs_ = 0;
    return true;
}

void ProgressionController::LoadBeforeSystems(const std::filesystem::path& path) {
    path_ = path;
    const auto result = core::LoadSaveData(path_, [this](std::string_view message) {
        logger_.Write(util::LogLevel::Warning, message);
    });
    data_ = result.data;
    writeAllowed_ = result.writeAllowed;
    if (result.status == core::SaveLoadStatus::Missing ||
        result.status == core::SaveLoadStatus::CorruptRecoveredDefaults) {
        data_.hiddenBlood = std::clamp(config_.feeding.initialBlood, 0.0, 100.0);
    }
    baseEncounterEnabled_ = config_.encounter.enabled;
    core::ApplyProgressionTuning(config_, data_);
    config_.feeding.initialBlood = std::clamp(data_.hiddenBlood, 0.0, 100.0);
    RefreshEncounterGate();
    lastSnapshot_ = core::SerializeSaveData(data_);
    logger_.Write(util::LogLevel::Info,
        "Progression/save state loaded; no custom progression HUD is enabled.");
}

void ProgressionController::ApplyAfterConfigReload() noexcept {
    baseEncounterEnabled_ = config_.encounter.enabled;
    core::ApplyProgressionTuning(config_, data_);
    RefreshEncounterGate();
}

void ProgressionController::RefreshEncounterGate() noexcept {
    if (data_.saintDenisCooldownUntilGameSeconds <= 0) {
        encounterGateActive_ = false;
        config_.encounter.enabled = baseEncounterEnabled_;
        return;
    }
    const auto now = encounterApi_.GameSecondsSinceBaseYear();
    if (!encounter_math::CooldownExpired(now, data_.saintDenisCooldownUntilGameSeconds)) {
        encounterGateActive_ = true;
        config_.encounter.enabled = false;
        return;
    }
    if (encounterGateActive_) {
        logger_.Write(util::LogLevel::Info,
            "Saved Saint Denis cooldown expired; encounter eligibility restored.");
    }
    encounterGateActive_ = false;
    data_.saintDenisCooldownUntilGameSeconds = 0;
    config_.encounter.enabled = baseEncounterEnabled_;
}

void ProgressionController::Capture() noexcept {
    data_.hiddenBlood = std::clamp(feeding_.HiddenResourceValue(), 0.0, 100.0);
    data_.saintDenisCompleted = data_.saintDenisCompleted || encounter_.ResolvedThisSession();
    const auto cooldown = encounter_.CooldownUntilGameSeconds();
    if (cooldown > 0) data_.saintDenisCooldownUntilGameSeconds = cooldown;
    core::NormalizeSaveData(data_);
}

void ProgressionController::Checkpoint(bool force) noexcept {
    if (!writeAllowed_ || path_.empty()) return;
    try {
        Capture();
        const auto snapshot = core::SerializeSaveData(data_);
        if (!force && snapshot == lastSnapshot_) return;
        if (core::StoreSaveDataAtomicish(path_, data_, [this](std::string_view message) {
                logger_.Write(util::LogLevel::Warning, message);
            })) {
            lastSnapshot_ = snapshot;
        }
    } catch (...) {
        logger_.Write(util::LogLevel::Error,
            "Progression checkpoint failed; gameplay continues with in-memory state.");
    }
}

void ProgressionController::Update(const core::FrameContext& frame) {
    RefreshEncounterGate();
    if (frame.nowMs < nextCheckMs_) return;
    nextCheckMs_ = frame.nowMs + 2000;
    Checkpoint(false);
}

void ProgressionController::Cancel() noexcept {
    Checkpoint(true);
}

void ProgressionController::Shutdown() noexcept {
    Checkpoint(true);
}

} // namespace nightwalker::systems
