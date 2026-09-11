#pragma once
#include <filesystem>
namespace nightwalker::game {
class GameContext final {
public:
 bool Initialize(std::filesystem::path modulePath);
 void Reset();
 const std::filesystem::path& ModulePath() const noexcept { return modulePath_; }
 const std::filesystem::path& PluginDirectory() const noexcept { return pluginDirectory_; }
private:
 std::filesystem::path modulePath_{};
 std::filesystem::path pluginDirectory_{};
};
}
