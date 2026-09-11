#include "nightwalker/systems/ShadowstepPresentationSettings.h"

#include <algorithm>
#include <cctype>
#include <charconv>
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
template <typename T>
bool ParseNumber(std::string_view text, T& out) {
    const std::string value = Trim(std::string(text));
    if (value.empty()) return false;
    T parsed{};
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return false;
    out = parsed;
    return true;
}
bool ParseBool(std::string_view text, bool& out) {
    const std::string value = Lower(Trim(std::string(text)));
    if (value == "true" || value == "1" || value == "yes" || value == "on") { out = true; return true; }
    if (value == "false" || value == "0" || value == "no" || value == "off") { out = false; return true; }
    return false;
}
void Warn(const PresentationDiagnosticSink& sink, std::string message) {
    if (sink) sink(message);
}
template <typename T>
void Clamp(T& value, T low, T high, const char* name, const PresentationDiagnosticSink& sink) {
    const T before = value;
    value = std::clamp(value, low, high);
    if (value != before) Warn(sink, std::string(name) + " was clamped to its safe Phase 4 range.");
}
}

ShadowstepPresentationSettings LoadShadowstepPresentationSettings(
    const std::filesystem::path& path, PresentationDiagnosticSink diagnostics) {
    ShadowstepPresentationSettings settings{};
    std::ifstream stream(path);
    if (!stream) return settings;

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
        if (section != "shadowstep") continue;
        const auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        const std::string key = Lower(Trim(line.substr(0, equals)));
        const std::string value = Trim(line.substr(equals + 1));
        bool handled = true;
        bool parsed = true;
        if (key == "disappearms") parsed = ParseNumber(value, settings.disappearMs);
        else if (key == "arrivalcarrymeters") parsed = ParseNumber(value, settings.carryMeters);
        else if (key == "arrivalcarryms") parsed = ParseNumber(value, settings.carryMs);
        else if (key == "meleebufferms" || key == "attackbufferms") parsed = ParseNumber(value, settings.meleeBufferMs);
        else if (key == "statetimeoutms") parsed = ParseNumber(value, settings.stateTimeoutMs);
        else if (key == "smokefx") parsed = ParseBool(value, settings.smokeFx);
        else handled = false;
        if (handled && !parsed) {
            Warn(diagnostics, "Invalid Phase 4 Shadowstep value at line " + std::to_string(lineNumber) + "; keeping the safe default.");
        }
    }

    Clamp(settings.disappearMs, std::uint32_t{80}, std::uint32_t{130}, "Shadowstep.DisappearMs", diagnostics);
    Clamp(settings.carryMeters, 0.0F, 2.0F, "Shadowstep.ArrivalCarryMeters", diagnostics);
    Clamp(settings.carryMs, std::uint32_t{80}, std::uint32_t{250}, "Shadowstep.ArrivalCarryMs", diagnostics);
    Clamp(settings.meleeBufferMs, std::uint32_t{80}, std::uint32_t{500}, "Shadowstep.MeleeBufferMs", diagnostics);
    Clamp(settings.stateTimeoutMs, std::uint32_t{300}, std::uint32_t{3000}, "Shadowstep.StateTimeoutMs", diagnostics);
    return settings;
}

} // namespace nightwalker::systems
