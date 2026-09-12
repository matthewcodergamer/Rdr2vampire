#include "nightwalker/systems/SaintDenisSettingsLoader.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace nightwalker::systems {
namespace {
std::string Trim(std::string value) {
    auto keep = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), keep));
    value.erase(std::find_if(value.rbegin(), value.rend(), keep).base(), value.end());
    return value;
}
std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
template <class T> bool ParseNumber(std::string_view text, T& out) {
    const std::string s = Trim(std::string(text));
    if (s.empty()) return false;
    T parsed{};
    const auto r = std::from_chars(s.data(), s.data() + s.size(), parsed);
    if (r.ec != std::errc{} || r.ptr != s.data() + s.size()) return false;
    out = parsed;
    return true;
}
template <class T> void Clamp(T& value, T lo, T hi, const char* name,
                              const core::Config::DiagnosticSink& diagnostics) {
    const T before = value;
    value = std::clamp(value, lo, hi);
    if (value != before && diagnostics) diagnostics(std::string(name) + " was outside the safe range and was clamped.");
}
void Invalid(const core::Config::DiagnosticSink& diagnostics, std::size_t line, std::string_view key) {
    if (diagnostics) diagnostics("Invalid Saint Denis encounter value at line " +
        std::to_string(line) + " for " + std::string(key) + "; previous/default value retained.");
}
}

void LoadSaintDenisSettings(const std::filesystem::path& iniPath,
                            core::EncounterSettings& settings,
                            const core::Config::DiagnosticSink& diagnostics) {
    std::ifstream input(iniPath);
    if (!input) return;

    std::string section;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = Trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = Lower(Trim(line.substr(1, line.size() - 2)));
            continue;
        }
        if (section != "encounter" && section != "encounter.saintdenis") continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = Lower(Trim(line.substr(0, eq)));
        const std::string value = Trim(line.substr(eq + 1));
        bool handled = true;
        bool parsed = true;
        if (key == "centerx") parsed = ParseNumber(value, settings.centerX);
        else if (key == "centery") parsed = ParseNumber(value, settings.centerY);
        else if (key == "centerz") parsed = ParseNumber(value, settings.centerZ);
        else if (key == "triggerradius") parsed = ParseNumber(value, settings.triggerRadius);
        else if (key == "abortradius") parsed = ParseNumber(value, settings.abortRadius);
        else if (key == "spawnmindistance") parsed = ParseNumber(value, settings.spawnMinDistance);
        else if (key == "spawnmaxdistance") parsed = ParseNumber(value, settings.spawnMaxDistance);
        else if (key == "confrontationdistance") parsed = ParseNumber(value, settings.confrontationDistance);
        else if (key == "eligibilitypollms") parsed = ParseNumber(value, settings.eligibilityPollMs);
        else if (key == "omendurationms") parsed = ParseNumber(value, settings.omenDurationMs);
        else if (key == "omenpulsems") parsed = ParseNumber(value, settings.omenPulseMs);
        else if (key == "spawnretryms") parsed = ParseNumber(value, settings.spawnRetryMs);
        else if (key == "spawntimeoutms") parsed = ParseNumber(value, settings.spawnTimeoutMs);
        else if (key == "stalkingms") parsed = ParseNumber(value, settings.stalkingMs);
        else if (key == "confrontationms") parsed = ParseNumber(value, settings.confrontationMs);
        else if (key == "leavegracems") parsed = ParseNumber(value, settings.leaveGraceMs);
        else if (key == "resolutionholdms") parsed = ParseNumber(value, settings.resolutionHoldMs);
        else if (key == "abortcooldownminutes") parsed = ParseNumber(value, settings.abortCooldownMinutes);
        else handled = false;
        if (handled && !parsed) Invalid(diagnostics, lineNumber, key);
    }

    Clamp(settings.centerX, -10000.0, 10000.0, "Encounter.CenterX", diagnostics);
    Clamp(settings.centerY, -10000.0, 10000.0, "Encounter.CenterY", diagnostics);
    Clamp(settings.centerZ, -1000.0, 2000.0, "Encounter.CenterZ", diagnostics);
    Clamp(settings.triggerRadius, 20.0, 250.0, "Encounter.TriggerRadius", diagnostics);
    Clamp(settings.abortRadius, 30.0, 400.0, "Encounter.AbortRadius", diagnostics);
    if (settings.abortRadius < settings.triggerRadius + 10.0) {
        settings.abortRadius = settings.triggerRadius + 10.0;
        if (diagnostics) diagnostics("Encounter.AbortRadius was raised above TriggerRadius.");
    }
    Clamp(settings.spawnMinDistance, 8.0, 60.0, "Encounter.SpawnMinDistance", diagnostics);
    Clamp(settings.spawnMaxDistance, 15.0, 100.0, "Encounter.SpawnMaxDistance", diagnostics);
    if (settings.spawnMaxDistance < settings.spawnMinDistance + 5.0) {
        settings.spawnMaxDistance = settings.spawnMinDistance + 5.0;
        if (diagnostics) diagnostics("Encounter.SpawnMaxDistance was raised above SpawnMinDistance.");
    }
    Clamp(settings.confrontationDistance, 5.0, 40.0, "Encounter.ConfrontationDistance", diagnostics);
    Clamp(settings.eligibilityPollMs, 200, 5000, "Encounter.EligibilityPollMs", diagnostics);
    Clamp(settings.omenDurationMs, 500, 10000, "Encounter.OmenDurationMs", diagnostics);
    Clamp(settings.omenPulseMs, 300, 3000, "Encounter.OmenPulseMs", diagnostics);
    Clamp(settings.spawnRetryMs, 200, 3000, "Encounter.SpawnRetryMs", diagnostics);
    Clamp(settings.spawnTimeoutMs, 2000, 20000, "Encounter.SpawnTimeoutMs", diagnostics);
    Clamp(settings.stalkingMs, 1000, 20000, "Encounter.StalkingMs", diagnostics);
    Clamp(settings.confrontationMs, 300, 3000, "Encounter.ConfrontationMs", diagnostics);
    Clamp(settings.leaveGraceMs, 1000, 30000, "Encounter.LeaveGraceMs", diagnostics);
    Clamp(settings.resolutionHoldMs, 0, 5000, "Encounter.ResolutionHoldMs", diagnostics);
    Clamp(settings.abortCooldownMinutes, 1, 120, "Encounter.AbortCooldownMinutes", diagnostics);
}

} // namespace nightwalker::systems
