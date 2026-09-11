#include "nightwalker/game/GameContext.h"
#include <utility>

namespace nightwalker::game {
bool GameContext::Initialize(std::filesystem::path modulePath) {
    Reset();
    if (modulePath.empty()) {
        return false;
    }
    modulePath_ = std::move(modulePath);
    pluginDirectory_ = modulePath_.parent_path();
    return !pluginDirectory_.empty();
}

void GameContext::Reset() {
    modulePath_.clear();
    pluginDirectory_.clear();
}
}
