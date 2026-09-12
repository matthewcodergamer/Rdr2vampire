#pragma once
#include <Windows.h>
#include <cstdint>
#include <string_view>
#include <vector>
#include "nightwalker/core/Config.h"
#include "nightwalker/core/DebugInput.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/core/SafetyWatchdog.h"
#include "nightwalker/util/Timing.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameCombatApi.h"
#include "nightwalker/game/GameContext.h"
#include "nightwalker/game/GameMovementApi.h"
#include "nightwalker/game/GamePresentationApi.h"
#include "nightwalker/systems/DebugVampireSpawner.h"
#include "nightwalker/systems/MovementController.h"
#include "nightwalker/systems/ShadowstepController.h"
#include "nightwalker/systems/VampireAIController.h"
#include "nightwalker/util/Logger.h"
namespace nightwalker::core {
class Runtime final {
public:
 Runtime();~Runtime()noexcept;Runtime(const Runtime&)=delete;Runtime&operator=(const Runtime&)=delete;
 bool Initialize(HMODULE moduleHandle);void Tick();void Shutdown()noexcept;bool IsInitialized()const noexcept{return initialized_;}
private:
 void ReloadConfig();void CancelSystems()noexcept;void RestoreOwnedState(std::string_view reason)noexcept;
 game::GameContext gameContext_{};util::Logger logger_{};Config config_{};SafetyWatchdog watchdog_{};DebugInput debugInput_{};game::GameApi gameApi_{};game::GameCombatApi gameCombatApi_{};game::GameMovementApi gameMovementApi_{};game::GamePresentationApi gamePresentationApi_{};systems::DebugVampireSpawner debugVampireSpawner_;systems::ShadowstepController shadowstepController_;systems::VampireAIController vampireAiController_;systems::MovementController movementController_;std::vector<ILifecycleSystem*> systems_{};util::Deadline debugDebounce_{};std::uint64_t tickCount_{0};std::uint64_t lastTickMs_{0};bool unsafeState_{false};bool initialized_{false};
};
}
