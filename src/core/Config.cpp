#include "nightwalker/core/Config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <sstream>
#include <string>

namespace nightwalker::core {
namespace {

constexpr int kDebugSpawnKey = 0x77;   // F8
constexpr int kDebugDespawnKey = 0x78; // F9
constexpr int kDefaultShadowstepKey = 0x76; // F7
constexpr int kDefaultReloadKey = 0x79;     // F10
constexpr int kDefaultRestoreKey = 0x7A;    // F11

std::string Trim(std::string value) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

void Warn(const Config::DiagnosticSink& sink, std::string message) {
    if (sink) sink(message);
}

bool ParseBool(std::string_view text, bool& value) {
    const std::string lower = Lower(Trim(std::string(text)));
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") { value = true; return true; }
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off") { value = false; return true; }
    return false;
}

template <typename T>
bool ParseNumber(std::string_view text, T& value) {
    const std::string trimmed = Trim(std::string(text));
    if (trimmed.empty()) return false;
    T parsed{};
    const auto result = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != trimmed.data() + trimmed.size()) return false;
    value = parsed;
    return true;
}

bool ParseHotkey(std::string_view text, int& value) {
    std::string trimmed = Trim(std::string(text));
    if (trimmed.empty()) return false;
    int base = 10;
    const char* begin = trimmed.c_str();
    if (trimmed.size() > 2 && trimmed[0] == '0' && (trimmed[1] == 'x' || trimmed[1] == 'X')) {
        base = 16;
        begin += 2;
    }
    unsigned parsed = 0;
    const char* end = trimmed.c_str() + trimmed.size();
    const auto result = std::from_chars(begin, end, parsed, base);
    if (result.ec != std::errc{} || result.ptr != end || parsed > 255) return false;
    value = static_cast<int>(parsed);
    return true;
}

template <typename T>
void Clamp(T& value, T low, T high, const char* name, const Config::DiagnosticSink& sink) {
    const T original = value;
    value = std::clamp(value, low, high);
    if (value != original) Warn(sink, std::string(name) + " was outside the safe range and was clamped.");
}

void Invalid(const Config::DiagnosticSink& sink, std::size_t line, std::string_view key) {
    Warn(sink, "Invalid value at line " + std::to_string(line) + " for " + std::string(key) + "; using the previous/default value.");
}

bool DebugHotkeysCollide(const DebugSettings& debug) noexcept {
    if (debug.shadowstepHotkey == debug.reloadHotkey || debug.shadowstepHotkey == debug.restoreHotkey ||
        debug.reloadHotkey == debug.restoreHotkey) {
        return true;
    }
    const int reserved[] = {kDebugSpawnKey, kDebugDespawnKey};
    for (const int key : reserved) {
        if (debug.shadowstepHotkey == key || debug.reloadHotkey == key || debug.restoreHotkey == key) return true;
    }
    return false;
}

} // namespace

bool Config::IsFeatureEnabled(Feature feature) const noexcept {
    if (!enabled) return false;
    switch (feature) {
        case Feature::Shadowstep: return shadowstep.enabled;
        case Feature::Movement: return movement.enabled;
        case Feature::Feeding: return feeding.enabled;
        case Feature::Encounter: return encounter.enabled;
        case Feature::BossHud: return bossHud.enabled;
        default: return false;
    }
}

Config Config::Load(const std::filesystem::path& path, DiagnosticSink diagnostics) {
    std::ifstream stream(path);
    if (!stream) {
        Warn(diagnostics, "Nightwalker.ini was not found/readable; safe defaults are active.");
        return {};
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return Parse(buffer.str(), std::move(diagnostics));
}

Config Config::Parse(std::string_view text, DiagnosticSink diagnostics) {
    Config config{};
    std::istringstream stream{std::string(text)};
    std::string section;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(stream, line)) {
        ++lineNumber;
        line = Trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = Lower(Trim(line.substr(1, line.size() - 2)));
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            Warn(diagnostics, "Ignoring malformed config line " + std::to_string(lineNumber) + ".");
            continue;
        }

        const std::string key = Lower(Trim(line.substr(0, equals)));
        const std::string value = Trim(line.substr(equals + 1));
        bool handled = true;
        bool parsed = true;

        if (section == "general") {
            if (key == "enabled") parsed = ParseBool(value, config.enabled);
            else if (key == "debugmode") parsed = ParseBool(value, config.debug.enabled); // legacy alias
            else handled = false;
        } else if (section == "debug") {
            if (key == "enabled") parsed = ParseBool(value, config.debug.enabled);
            else if (key == "shadowstephotkey") parsed = ParseHotkey(value, config.debug.shadowstepHotkey);
            else if (key == "restorehotkey") parsed = ParseHotkey(value, config.debug.restoreHotkey);
            else if (key == "reloadhotkey") parsed = ParseHotkey(value, config.debug.reloadHotkey);
            else handled = false;
        } else if (section == "shadowstep") {
            if (key == "enabled") parsed = ParseBool(value, config.shadowstep.enabled);
            else if (key == "quickdistance") parsed = ParseNumber(value, config.shadowstep.quickDistance);
            else if (key == "aimdistance") parsed = ParseNumber(value, config.shadowstep.aimDistance);
            else if (key == "cooldownms") parsed = ParseNumber(value, config.shadowstep.cooldownMs);
            else if (key == "maxverticaldelta" || key == "maxverticalrise") parsed = ParseNumber(value, config.shadowstep.maxVerticalDelta);
            else if (key == "validationtimeoutms") parsed = ParseNumber(value, config.shadowstep.validationTimeoutMs);
            else if (key == "wallclearance") parsed = ParseNumber(value, config.shadowstep.wallClearance);
            else handled = false;
        } else if (section == "movement") {
            if (key == "enabled") parsed = ParseBool(value, config.movement.enabled);
            else if (key == "sprintmoverate") parsed = ParseNumber(value, config.movement.sprintMoveRate);
            else handled = false;
        } else if (section == "feeding") {
            if (key == "enabled") parsed = ParseBool(value, config.feeding.enabled);
            else if (key == "allownonlethal") parsed = ParseBool(value, config.feeding.allowNonLethal);
            else if (key == "allowanimalfeeding") parsed = ParseBool(value, config.feeding.allowAnimalFeeding);
            else handled = false;
        } else if (section == "encounter" || section == "encounter.saintdenis") {
            if (key == "enabled") parsed = ParseBool(value, config.encounter.enabled);
            else if (key == "starthour") parsed = ParseNumber(value, config.encounter.startHour);
            else if (key == "endhour") parsed = ParseNumber(value, config.encounter.endHour);
            else if (key == "respawncooldownhours") parsed = ParseNumber(value, config.encounter.respawnCooldownHours);
            else handled = false;
        } else if (section == "bosshud") {
            if (key == "enabled") parsed = ParseBool(value, config.bossHud.enabled);
            else if (key == "idleseconds") parsed = ParseNumber(value, config.bossHud.idleSeconds);
            else if (key == "fadeseconds") parsed = ParseNumber(value, config.bossHud.fadeSeconds);
            else if (key == "deathholdseconds") parsed = ParseNumber(value, config.bossHud.deathHoldSeconds);
            else if (key == "shownumerichealth") parsed = ParseBool(value, config.bossHud.showNumericHealth);
            else handled = false;
        } else {
            handled = false;
        }

        if (handled && !parsed) Invalid(diagnostics, lineNumber, key);
    }

    Clamp(config.shadowstep.quickDistance, 1.5, 12.0, "Shadowstep.QuickDistance", diagnostics);
    Clamp(config.shadowstep.aimDistance, 1.5, 20.0, "Shadowstep.AimDistance", diagnostics);
    Clamp(config.shadowstep.cooldownMs, 200, 5000, "Shadowstep.CooldownMs", diagnostics);
    Clamp(config.shadowstep.maxVerticalDelta, 0.25, 3.0, "Shadowstep.MaxVerticalDelta", diagnostics);
    Clamp(config.shadowstep.validationTimeoutMs, 50, 2000, "Shadowstep.ValidationTimeoutMs", diagnostics);
    Clamp(config.shadowstep.wallClearance, 0.40, 1.50, "Shadowstep.WallClearance", diagnostics);
    Clamp(config.movement.sprintMoveRate, 1.0, 2.0, "Movement.SprintMoveRate", diagnostics);
    Clamp(config.encounter.startHour, 0, 23, "Encounter.StartHour", diagnostics);
    Clamp(config.encounter.endHour, 0, 23, "Encounter.EndHour", diagnostics);
    Clamp(config.encounter.respawnCooldownHours, 1, 720, "Encounter.RespawnCooldownHours", diagnostics);
    Clamp(config.bossHud.idleSeconds, 1.0, 30.0, "BossHUD.IdleSeconds", diagnostics);
    Clamp(config.bossHud.fadeSeconds, 0.1, 3.0, "BossHUD.FadeSeconds", diagnostics);
    Clamp(config.bossHud.deathHoldSeconds, 0.0, 5.0, "BossHUD.DeathHoldSeconds", diagnostics);

    if (DebugHotkeysCollide(config.debug)) {
        Warn(diagnostics, "Debug hotkeys collided with F8/F9 or each other; restoring safe F7/F10/F11 defaults.");
        config.debug.shadowstepHotkey = kDefaultShadowstepKey;
        config.debug.reloadHotkey = kDefaultReloadKey;
        config.debug.restoreHotkey = kDefaultRestoreKey;
    }
    if (config.bossHud.showNumericHealth) {
        Warn(diagnostics, "BossHUD.ShowNumericHealth is locked off by DESIGN_LOCKS.md.");
        config.bossHud.showNumericHealth = false;
    }
    return config;
}

} // namespace nightwalker::core
