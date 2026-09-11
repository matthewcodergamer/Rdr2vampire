#pragma once

#include "nightwalker/core/Config.h"

namespace nightwalker::core::config_internal {

void Validate(Config& config, const Config::DiagnosticSink& diagnostics);

} // namespace nightwalker::core::config_internal
