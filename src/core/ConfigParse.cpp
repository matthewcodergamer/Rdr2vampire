#include "nightwalker/core/Config.h"
#include "ConfigInternal.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <string>

namespace nightwalker::core {
namespace {

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
    const std::string trimmed = Trim(std::string(text));
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

void Invalid(const Config::DiagnosticSink& diagnostics, std::size_t line, std::string_view key) {
    if (diagnostics) {
        diagnostics("Invalid value at line " + std::to_string(line) + " for " + std::string(key) +
                    "; using the previous/default value.");
    }
}

} // namespace

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
            if (diagnostics) diagnostics("Ignoring malformed config line " + std::to_string(lineNumber) + ".");
            continue;
        }

        const std::string key = Lower(Trim(line.substr(0, equals)));
        const std::string value = Trim(line.substr(equals + 1));
        bool handled = true;
        bool parsed = true;

        if (section == "general") {
            if (key == "enabled") parsed = ParseBool(value, config.enabled);
            else if (key == "debugmode") parsed = ParseBool(value, config.debug.enabled);
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
        } else if (section == "vampireai") {
            if (key == "enabled") parsed = ParseBool(value, config.vampireAi.enabled);
            else if (key == "shadowstepmindistance") parsed = ParseNumber(value, config.vampireAi.shadowstepMinDistance);
            else if (key == "shadowstepmaxdistance") parsed = ParseNumber(value, config.vampireAi.shadowstepMaxDistance);
            else if (key == "strikingrange") parsed = ParseNumber(value, config.vampireAi.strikingRange);
            else if (key == "predictionms") parsed = ParseNumber(value, config.vampireAi.predictionMs);
            else if (key == "decisionintervalms") parsed = ParseNumber(value, config.vampireAi.decisionIntervalMs);
            else if (key == "shadowstepcooldownms") parsed = ParseNumber(value, config.vampireAi.shadowstepCooldownMs);
            else if (key == "telegraphms") parsed = ParseNumber(value, config.vampireAi.telegraphMs);
            else if (key == "recoveryms") parsed = ParseNumber(value, config.vampireAi.recoveryMs);
            else if (key == "evadecooldownms") parsed = ParseNumber(value, config.vampireAi.evadeCooldownMs);
            else if (key == "retreatspeedthreshold") parsed = ParseNumber(value, config.vampireAi.retreatSpeedThreshold);
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

    config_internal::Validate(config, diagnostics);
    return config;
}

} // namespace nightwalker::core
