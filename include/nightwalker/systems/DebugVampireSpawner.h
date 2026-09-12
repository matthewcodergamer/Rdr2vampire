#pragma once
#include <cstdint>
#include <string_view>
#include "nightwalker/core/Config.h"
#include "nightwalker/core/ILifecycleSystem.h"
#include "nightwalker/game/GameApi.h"
#include "nightwalker/game/ModelStreamRequest.h"
#include "nightwalker/systems/BossActorRegistry.h"
#include "nightwalker/util/Logger.h"
namespace nightwalker::systems {
class DebugVampireSpawner final:public core::ILifecycleSystem{
public:
 static constexpr game::ModelHash kVampireModel=kSaintDenisVampireModel;
 DebugVampireSpawner(game::IGameApi& api,BossActorRegistry& registry,util::Logger& logger,const core::Config& config)noexcept;
 std::string_view Name()const noexcept override{return "DebugVampireSpawner";}
 bool Initialize()override;void Update(const core::FrameContext& frame)override;void Cancel()noexcept override;void Shutdown()noexcept override;
 void RequestSpawn(std::uint64_t nowMs)noexcept;void RequestDespawn()noexcept;game::PedHandle OwnedPed()const noexcept{return ownedPed_;}
private:
 struct BoolRef{const bool* value{};operator bool()const noexcept{return value&&*value;}};
 struct LocalSettings{struct Debug{BoolRef enabled{};std::uint64_t modelLoadTimeoutMs{5000};}debug{};explicit LocalSettings(const core::Config& c)noexcept{debug.enabled.value=&c.debug.enabled;}};
 enum class State{Idle,LoadingModel,Spawned};
 bool FindSpawnPoint(game::PedHandle player,game::Vec3& position,float& heading)const noexcept;
 bool SpawnLoadedModel(std::uint64_t nowMs)noexcept;void ResetRequest()noexcept;
 game::IGameApi& api_;BossActorRegistry& registry_;util::Logger& logger_;LocalSettings config_;game::ModelStreamRequest modelRequest_{};game::PedHandle ownedPed_{0};State state_{State::Idle};
};
}
