#include "nightwalker/core/Config.h"

#include <fstream>
#include <sstream>
#include <utility>

namespace nightwalker::core {

bool Config::IsFeatureEnabled(Feature feature) const noexcept {
    if (!enabled) return false;
    switch (feature) {
        case Feature::Shadowstep: return shadowstep.enabled;
        case Feature::Movement: return movement.enabled;
        case Feature::Feeding: return feeding.enabled;
        case Feature::Encounter: return encounter.enabled;
        case Feature::VampireAi: return vampireAi.enabled;
        case Feature::BossHud: return bossHud.enabled;
        default: return false;
    }
}

Config Config::Load(const std::filesystem::path& path, DiagnosticSink diagnostics) {
    std::ifstream stream(path);
    if (!stream) {
        if (diagnostics) diagnostics("Nightwalker.ini was not found/readable; safe defaults are active.");
        return {};
    }

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return Parse(buffer.str(), std::move(diagnostics));
}

} // namespace nightwalker::core
