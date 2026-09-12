#pragma once

#include <filesystem>
#include <functional>
#include <string_view>

#include "nightwalker/core/Config.h"

namespace nightwalker::narrative {

using NarrativeSettingsDiagnosticSink = std::function<void(std::string_view)>;

void LoadNarrativeSettings(const std::filesystem::path& path,
                           core::NarrativeSettings& settings,
                           NarrativeSettingsDiagnosticSink diagnostics = {});

} // namespace nightwalker::narrative
