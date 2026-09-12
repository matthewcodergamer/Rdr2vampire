#include "nightwalker/core/SaveData.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>

namespace nightwalker::core {
namespace {

std::string Trim(std::string value) {
    const auto notSpace = [](unsigned char c) { return c != ' ' && c != '\t' && c != '\r' && c != '\n'; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    });
    return value;
}

template <class T>
bool ParseInteger(std::string_view text, T& value) {
    const std::string clean = Trim(std::string(text));
    if (clean.empty()) return false;
    T parsed{};
    const auto result = std::from_chars(clean.data(), clean.data() + clean.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != clean.data() + clean.size()) return false;
    value = parsed;
    return true;
}

bool ParseDouble(std::string_view text, double& value) {
    const std::string clean = Trim(std::string(text));
    if (clean.empty()) return false;
    double parsed{};
    const auto result = std::from_chars(clean.data(), clean.data() + clean.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != clean.data() + clean.size() || !std::isfinite(parsed)) return false;
    value = parsed;
    return true;
}

bool ParseBool(std::string_view text, bool& value) {
    const std::string clean = Lower(Trim(std::string(text)));
    if (clean == "true" || clean == "1" || clean == "yes" || clean == "on") { value = true; return true; }
    if (clean == "false" || clean == "0" || clean == "no" || clean == "off") { value = false; return true; }
    return false;
}

bool ReadSchemaVersion(std::string_view text, int& version) {
    std::istringstream stream{std::string(text)};
    std::string line;
    while (std::getline(stream, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const auto key = Lower(Trim(line.substr(0, eq)));
        if (key == "schemaversion" || key == "version") return ParseInteger(line.substr(eq + 1), version);
    }
    return false;
}

std::string ReadText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Diagnostic(const SaveDiagnosticSink& sink, std::string_view message) {
    if (sink) sink(message);
}

template <class T>
void ClampWithDiagnostic(T& value, T low, T high, std::string_view label,
                         const SaveDiagnosticSink& sink) {
    const T before = value;
    value = std::clamp(value, low, high);
    if (value != before) Diagnostic(sink, std::string("Save value clamped: ") + std::string(label));
}

bool ParseKnownFields(std::string_view text, int sourceVersion, NightwalkerSaveData& out,
                      const SaveDiagnosticSink& diagnostics) {
    NightwalkerSaveData data{};
    data.schemaVersion = kNightwalkerSaveSchemaVersion;
    std::istringstream stream{std::string(text)};
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(stream, line)) {
        ++lineNumber;
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            Diagnostic(diagnostics, "Ignoring malformed save line " + std::to_string(lineNumber) + ".");
            continue;
        }
        const auto key = Lower(Trim(line.substr(0, eq)));
        const auto value = Trim(line.substr(eq + 1));
        bool recognized = true;
        bool parsed = true;

        if (key == "schemaversion" || key == "version") {
            // Already validated before field parsing.
        } else if (key == "encounter.saintdenis.completed" || (sourceVersion == 0 && key == "bosscompleted")) {
            parsed = ParseBool(value, data.saintDenisCompleted);
        } else if (key == "encounter.saintdenis.cooldownuntilgameseconds" ||
                   (sourceVersion == 0 && key == "cooldownuntil")) {
            parsed = ParseInteger(value, data.saintDenisCooldownUntilGameSeconds);
        } else if (key == "resource.blood" || (sourceVersion == 0 && key == "hunger")) {
            parsed = ParseDouble(value, data.hiddenBlood);
        } else if (key == "progression.points" || (sourceVersion == 0 && key == "points")) {
            parsed = ParseInteger(value, data.progressionPoints);
        } else if (key == "unlock.targetedshadowstep") {
            parsed = ParseBool(value, data.unlocks.targetedShadowstep);
        } else if (key == "unlock.enhancedflanklogic") {
            parsed = ParseBool(value, data.unlocks.enhancedFlankLogic);
        } else if (key == "unlock.regeneration") {
            parsed = ParseBool(value, data.unlocks.regeneration);
        } else if (key == "tuning.shadowsteprangemultiplier") {
            parsed = ParseDouble(value, data.tuning.shadowstepRangeMultiplier);
        } else if (key == "tuning.shadowstepcooldownmultiplier") {
            parsed = ParseDouble(value, data.tuning.shadowstepCooldownMultiplier);
        } else if (key == "tuning.feedingefficiencymultiplier") {
            parsed = ParseDouble(value, data.tuning.feedingEfficiencyMultiplier);
        } else if (key == "tuning.sprintmultiplier") {
            parsed = ParseDouble(value, data.tuning.sprintMultiplier);
        } else if (key == "tuning.throwstrengthmultiplier") {
            parsed = ParseDouble(value, data.tuning.throwStrengthMultiplier);
        } else {
            recognized = false;
        }

        if (recognized && !parsed) {
            Diagnostic(diagnostics, "Invalid save value at line " + std::to_string(lineNumber) +
                                    " for " + key + "; using default value.");
        }
    }

    NormalizeSaveData(data, diagnostics);
    out = data;
    // A recognized bad field recovers independently to its default. Schema validity
    // is the file-level gate, so a valid supported schema remains loadable here.
    return true;
}

bool TryLoadOne(const std::filesystem::path& path, NightwalkerSaveData& out,
                int& sourceVersion, const SaveDiagnosticSink& diagnostics) {
    const std::string text = ReadText(path);
    if (text.empty()) return false;
    if (!ReadSchemaVersion(text, sourceVersion)) {
        Diagnostic(diagnostics, "Nightwalker save is missing a valid schema version.");
        return false;
    }
    if (sourceVersion > kNightwalkerSaveSchemaVersion) return false;
    if (sourceVersion < 0) return false;
    return ParseKnownFields(text, sourceVersion, out, diagnostics);
}

} // namespace

void NormalizeSaveData(NightwalkerSaveData& data, SaveDiagnosticSink diagnostics) {
    data.schemaVersion = kNightwalkerSaveSchemaVersion;
    data.saintDenisCooldownUntilGameSeconds = std::max<std::int64_t>(0, data.saintDenisCooldownUntilGameSeconds);
    ClampWithDiagnostic(data.hiddenBlood, 0.0, 100.0, "resource.blood", diagnostics);
    ClampWithDiagnostic(data.progressionPoints, 0, 999, "progression.points", diagnostics);
    ClampWithDiagnostic(data.tuning.shadowstepRangeMultiplier, 1.0, 1.25,
                        "tuning.shadowstepRangeMultiplier", diagnostics);
    ClampWithDiagnostic(data.tuning.shadowstepCooldownMultiplier, 0.65, 1.0,
                        "tuning.shadowstepCooldownMultiplier", diagnostics);
    ClampWithDiagnostic(data.tuning.feedingEfficiencyMultiplier, 1.0, 1.50,
                        "tuning.feedingEfficiencyMultiplier", diagnostics);
    ClampWithDiagnostic(data.tuning.sprintMultiplier, 1.0, 1.08,
                        "tuning.sprintMultiplier", diagnostics);
    ClampWithDiagnostic(data.tuning.throwStrengthMultiplier, 1.0, 1.15,
                        "tuning.throwStrengthMultiplier", diagnostics);
}

std::string SerializeSaveData(const NightwalkerSaveData& input) {
    NightwalkerSaveData data = input;
    NormalizeSaveData(data);
    std::ostringstream out;
    out << "# Nightwalker-owned save data. Do not copy this into an RDR2 save file.\n";
    out << "schemaVersion=" << kNightwalkerSaveSchemaVersion << '\n';
    out << "encounter.saintdenis.completed=" << (data.saintDenisCompleted ? "true" : "false") << '\n';
    out << "encounter.saintdenis.cooldownUntilGameSeconds=" << data.saintDenisCooldownUntilGameSeconds << '\n';
    out << std::fixed << std::setprecision(4);
    out << "resource.blood=" << data.hiddenBlood << '\n';
    out << "progression.points=" << data.progressionPoints << '\n';
    out << "unlock.targetedShadowstep=" << (data.unlocks.targetedShadowstep ? "true" : "false") << '\n';
    out << "unlock.enhancedFlankLogic=" << (data.unlocks.enhancedFlankLogic ? "true" : "false") << '\n';
    out << "unlock.regeneration=" << (data.unlocks.regeneration ? "true" : "false") << '\n';
    out << "tuning.shadowstepRangeMultiplier=" << data.tuning.shadowstepRangeMultiplier << '\n';
    out << "tuning.shadowstepCooldownMultiplier=" << data.tuning.shadowstepCooldownMultiplier << '\n';
    out << "tuning.feedingEfficiencyMultiplier=" << data.tuning.feedingEfficiencyMultiplier << '\n';
    out << "tuning.sprintMultiplier=" << data.tuning.sprintMultiplier << '\n';
    out << "tuning.throwStrengthMultiplier=" << data.tuning.throwStrengthMultiplier << '\n';
    return out.str();
}

bool ParseSaveData(std::string_view text, NightwalkerSaveData& data, SaveDiagnosticSink diagnostics) {
    int version = -1;
    if (!ReadSchemaVersion(text, version)) {
        Diagnostic(diagnostics, "Nightwalker save has no readable schemaVersion; defaults will be used.");
        data = NightwalkerSaveData{};
        return false;
    }
    if (version > kNightwalkerSaveSchemaVersion) {
        Diagnostic(diagnostics, "Nightwalker save was written by a newer schema; refusing to downgrade it.");
        data = NightwalkerSaveData{};
        return false;
    }
    if (version < 0) {
        Diagnostic(diagnostics, "Nightwalker save schema is invalid; defaults will be used.");
        data = NightwalkerSaveData{};
        return false;
    }
    const bool ok = ParseKnownFields(text, version, data, diagnostics);
    if (ok && version < kNightwalkerSaveSchemaVersion) {
        Diagnostic(diagnostics, "Nightwalker save migrated to schema version " +
                                std::to_string(kNightwalkerSaveSchemaVersion) + ".");
    }
    return ok;
}

SaveLoadResult LoadSaveData(const std::filesystem::path& path, SaveDiagnosticSink diagnostics) {
    SaveLoadResult result{};
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        const auto backup = std::filesystem::path(path.string() + ".bak");
        const auto temp = std::filesystem::path(path.string() + ".tmp");
        int sourceVersion = -1;
        if (std::filesystem::exists(backup, ec) && TryLoadOne(backup, result.data, sourceVersion, diagnostics)) {
            result.status = SaveLoadStatus::RecoveredBackup;
            Diagnostic(diagnostics, "Nightwalker save recovered from backup because the primary file was missing.");
            return result;
        }
        if (std::filesystem::exists(temp, ec) && TryLoadOne(temp, result.data, sourceVersion, diagnostics)) {
            result.status = SaveLoadStatus::RecoveredBackup;
            Diagnostic(diagnostics, "Nightwalker save recovered from a completed temp write.");
            return result;
        }
        result.status = SaveLoadStatus::Missing;
        return result;
    }

    const std::string primaryText = ReadText(path);
    int primaryVersion = -1;
    if (ReadSchemaVersion(primaryText, primaryVersion) && primaryVersion > kNightwalkerSaveSchemaVersion) {
        result.status = SaveLoadStatus::UnsupportedFutureVersion;
        result.writeAllowed = false;
        Diagnostic(diagnostics, "Nightwalker save schema is newer than this build; save writes are disabled to avoid data loss.");
        return result;
    }
    if (primaryVersion >= 0 && primaryVersion <= kNightwalkerSaveSchemaVersion &&
        ParseKnownFields(primaryText, primaryVersion, result.data, diagnostics)) {
        result.status = primaryVersion < kNightwalkerSaveSchemaVersion ? SaveLoadStatus::Migrated : SaveLoadStatus::Loaded;
        return result;
    }

    const auto backup = std::filesystem::path(path.string() + ".bak");
    int backupVersion = -1;
    if (std::filesystem::exists(backup, ec) && TryLoadOne(backup, result.data, backupVersion, diagnostics)) {
        result.status = SaveLoadStatus::RecoveredBackup;
        Diagnostic(diagnostics, "Primary Nightwalker save was corrupt; recovered the previous backup.");
        return result;
    }

    result.status = SaveLoadStatus::CorruptRecoveredDefaults;
    result.data = NightwalkerSaveData{};
    Diagnostic(diagnostics, "Nightwalker save was corrupt and no valid backup existed; recovered with safe defaults.");
    return result;
}

bool StoreSaveDataAtomicish(const std::filesystem::path& path, const NightwalkerSaveData& data,
                            SaveDiagnosticSink diagnostics) {
    std::error_code ec;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        Diagnostic(diagnostics, "Could not create Nightwalker save directory.");
        return false;
    }

    const auto temp = std::filesystem::path(path.string() + ".tmp");
    const auto backup = std::filesystem::path(path.string() + ".bak");
    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        if (!file) {
            Diagnostic(diagnostics, "Could not open Nightwalker save temp file for writing.");
            return false;
        }
        const std::string text = SerializeSaveData(data);
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        file.flush();
        if (!file.good()) {
            Diagnostic(diagnostics, "Nightwalker save temp write failed.");
            file.close();
            std::filesystem::remove(temp, ec);
            return false;
        }
    }

    std::filesystem::remove(backup, ec);
    ec.clear();
    const bool hadPrimary = std::filesystem::exists(path, ec);
    ec.clear();
    if (hadPrimary) {
        std::filesystem::rename(path, backup, ec);
        if (ec) {
            Diagnostic(diagnostics, "Could not rotate the previous Nightwalker save to backup.");
            std::filesystem::remove(temp, ec);
            return false;
        }
    }

    ec.clear();
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        Diagnostic(diagnostics, "Could not promote Nightwalker save temp file; attempting backup restore.");
        std::error_code restoreEc;
        if (hadPrimary && std::filesystem::exists(backup, restoreEc)) {
            restoreEc.clear();
            std::filesystem::rename(backup, path, restoreEc);
        }
        std::filesystem::remove(temp, restoreEc);
        return false;
    }

    std::filesystem::remove(backup, ec);
    return true;
}

void ApplyProgressionTuning(Config& config, const NightwalkerSaveData& input) noexcept {
    NightwalkerSaveData data = input;
    NormalizeSaveData(data);

    config.shadowstep.quickDistance = std::clamp(
        config.shadowstep.quickDistance * data.tuning.shadowstepRangeMultiplier, 1.0, 15.0);
    config.shadowstep.aimDistance = std::clamp(
        config.shadowstep.aimDistance * data.tuning.shadowstepRangeMultiplier, 1.0, 25.0);
    config.shadowstep.cooldownMs = std::clamp(
        static_cast<int>(std::lround(static_cast<double>(config.shadowstep.cooldownMs) *
                                     data.tuning.shadowstepCooldownMultiplier)),
        100, 10000);

    config.feeding.sipBloodGain = std::clamp(
        config.feeding.sipBloodGain * data.tuning.feedingEfficiencyMultiplier, 0.0, 100.0);
    config.feeding.drainBloodGain = std::clamp(
        config.feeding.drainBloodGain * data.tuning.feedingEfficiencyMultiplier, 0.0, 100.0);
    config.feeding.healthRestoreSip = std::clamp(
        static_cast<int>(std::lround(static_cast<double>(config.feeding.healthRestoreSip) *
                                     data.tuning.feedingEfficiencyMultiplier)),
        0, 100);
    config.feeding.healthRestoreDrain = std::clamp(
        static_cast<int>(std::lround(static_cast<double>(config.feeding.healthRestoreDrain) *
                                     data.tuning.feedingEfficiencyMultiplier)),
        0, 100);
}

void AdvanceDebugProgression(NightwalkerSaveData& data) noexcept {
    data.progressionPoints = std::min(10, data.progressionPoints + 1);
    const double p = static_cast<double>(data.progressionPoints);
    data.tuning.shadowstepRangeMultiplier = std::min(1.25, 1.0 + p * 0.025);
    data.tuning.shadowstepCooldownMultiplier = std::max(0.65, 1.0 - p * 0.035);
    data.tuning.feedingEfficiencyMultiplier = std::min(1.50, 1.0 + p * 0.05);
    data.tuning.sprintMultiplier = std::min(1.08, 1.0 + p * 0.008);
    data.tuning.throwStrengthMultiplier = std::min(1.15, 1.0 + p * 0.015);
    data.unlocks.targetedShadowstep = true;
    data.unlocks.enhancedFlankLogic = data.progressionPoints >= 3;
    data.unlocks.regeneration = data.progressionPoints >= 6;
    NormalizeSaveData(data);
}

} // namespace nightwalker::core
