#pragma once
#include <filesystem>
namespace nightwalker::game {
struct RuntimeState{bool playerValid=false;bool playerAlive=false;bool controlOn=false;bool missionActive=false;bool Unsafe()const noexcept{return !playerValid||!playerAlive||!controlOn||missionActive;}};
class GameContext final{public:bool Initialize(std::filesystem::path modulePath);void Reset();RuntimeState QueryRuntimeState()const noexcept;const std::filesystem::path& ModulePath()const noexcept{return modulePath_;}const std::filesystem::path& PluginDirectory()const noexcept{return pluginDirectory_;}private:std::filesystem::path modulePath_{};std::filesystem::path pluginDirectory_{};};
}
