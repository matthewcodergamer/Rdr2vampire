#pragma once

#include <filesystem>

#include "nightwalker/core/Config.h"

namespace nightwalker::systems {

void LoadSaintDenisSettings(
    const std::filesystem::path& iniPath,
    core::EncounterSettings& settings,
    const core::Config::DiagnosticSink& diagnostics = {});

} // namespace nightwalker::systems
