#pragma once
#include <Windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>
#include "nightwalker/core/Config.h"
#include "nightwalker/core/DebugInput.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/core/LongSessionGuard.h"
#include "nightwalker/core/SafetyWatchdog.h"
#include "nightwalker/util/Timing.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/GameBossBarApi.h"
#include "nightwalker/game/GameCombatApi.h"
#include "nightwalker/game/GameContext.h"
#include "nightwalker/game/GameConversationApi.h"
#include "nightwalker/game/GameEncounterApi.h"
#include "nightwalker/game/GameFeedingApi.h"
#include "nightwalker/game/GameMovementApi.h"
#include "nightwalker/game/GameNarrativeAudioApi.h"
#include "nightwalker/game/GamePhysicalApi.h"
#include "nightwalker/game/GamePresentationApi.h"
#include "nightwalker/narrative/NarrativeController.h"
#include "nightwalker/narrative/ReactiveConversationController.h"
#include "nightwalker/systems/BossActorRegistry.h"
#include "nightwalker/systems/DebugVampireSpawner.h"
#include "nightwalker/systems/FeedingController.h"
#include "nightwalker/systems/MovementController.h"
#include "nightwalker/systems/ProgressionController.h"
#include "nightwalker/systems/SaintDenisDirector.h"
#include "nightwalker/systems/ShadowstepController.h"
#include "nightwalker/systems/VampireAIController.h"
#include "nightwalker/systems/VampireCombatController.h"
#include "nightwalker/ui/BossHudController.h"
#include "nightwalker/util/Logger.h"
namespace nightwalker::core {
class Runtime final {
public:
 Runtime();~Runtime()noexcept;Runtime(const Runtime&)=delete;Runtime&operator=(const Runtime&)=delete;
 bool Initialize(HMODULE moduleHandle);void Tick();void Shutdown()noexcept;bool IsInitialized()const noexcept{return initialized_;}
private:
 struct SystemProfile final {std::uint64_t updates{0};std::uint64_t totalMicros{0};std::uint64_t maxMicros{0};};
 static constexpr std::size_t kMaxProfiledSystems=16;
 void ReloadConfig();void CancelSystems()noexcept;void RestoreOwnedState(std::string_view reason)noexcept;
 bool ValidateBossReference(std::uint64_t nowMs)noexcept;void UpdateSystems(const FrameContext& frame);void ResetPerformanceProfile()noexcept;void ReportPerformance(std::uint64_t nowMs);
 game::GameContext gameContext_{};util::Logger logger_{};Config config_{};SafetyWatchdog watchdog_{};LongSessionGuard sessionGuard_{};DebugInput debugInput_{};
 game::GameApi gameApi_{};game::GameBossBarApi gameBossBarApi_{};game::GameCombatApi gameCombatApi_{};game::GameConversationApi gameConversationApi_{};game::GameEncounterApi gameEncounterApi_{};game::GameFeedingApi gameFeedingApi_{};game::GameMovementApi gameMovementApi_{};game::SubtitleOnlyNarrativeAudioApi gameNarrativeAudioApi_{};game::GamePhysicalApi gamePhysicalApi_{};game::GamePresentationApi gamePresentationApi_{};
 systems::BossActorRegistry bossRegistry_{};
 ui::BossHudController bossHudController_;
 narrative::NarrativeController narrativeController_;
 narrative::ReactiveConversationController reactiveConversationController_;
 systems::DebugVampireSpawner debugVampireSpawner_;
 systems::ShadowstepController shadowstepController_;
 systems::FeedingController feedingController_;
 systems::VampireCombatController vampireCombatController_;
 systems::SaintDenisDirector saintDenisDirector_;
 systems::VampireAIController vampireAiController_;
 systems::MovementController movementController_;
 systems::ProgressionController progressionController_;
 std::vector<ILifecycleSystem*> systems_{};std::array<SystemProfile,kMaxProfiledSystems> systemProfiles_{};util::Deadline debugDebounce_{};std::uint64_t tickCount_{0};std::uint64_t lastTickMs_{0};std::uint64_t nextBossValidationMs_{0};std::uint64_t nextProfileReportMs_{0};bool initialized_{false};
};
}
