#include "nightwalker/systems/DebugVampireSpawner.h"
#include <array>
#include <cmath>
#include <string>
namespace nightwalker::systems {
namespace {
float DistSq(const game::Vec3&a,const game::Vec3&b)noexcept{float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
float NormalizeHeading(float h)noexcept{h=std::fmod(h,360.0F);return h<0.0F?h+360.0F:h;}
}
DebugVampireSpawner::DebugVampireSpawner(game::IGameApi&api,util::Logger&logger,const core::Config&config)noexcept:api_(api),logger_(logger),config_(config){}
bool DebugVampireSpawner::Initialize(){ownedPed_=0;state_=State::Idle;return true;}
void DebugVampireSpawner::RequestSpawn(std::uint64_t nowMs)noexcept{
 if(!config_.debug.enabled)return;
 if(ownedPed_!=0&&api_.EntityExists(ownedPed_)){if(api_.EntityModel(ownedPed_)==kVampireModel){logger_.Write(util::LogLevel::Debug,"Debug vampire already exists; duplicate spawn ignored.");return;}logger_.Write(util::LogLevel::Error,"Owned vampire handle was reused by another model; dropping stale ownership.");ownedPed_=0;state_=State::Idle;}
 if(state_==State::LoadingModel){logger_.Write(util::LogLevel::Debug,"Vampire model request already in progress.");return;}
 ownedPed_=0;
 auto status=modelRequest_.Begin(api_,kVampireModel,nowMs,(std::uint64_t)config_.debug.modelLoadTimeoutMs);
 if(status==game::ModelStreamStatus::InvalidModel){logger_.Write(util::LogLevel::Error,"cs_vampire model is unavailable or is not a ped model.");ResetRequest();return;}
 state_=State::LoadingModel;
 logger_.Write(util::LogLevel::Info,"Requested cs_vampire model for debug spawn.");
}
void DebugVampireSpawner::Update(const core::FrameContext&frame){
 if(state_==State::Spawned){if(ownedPed_!=0&&(!api_.EntityExists(ownedPed_)||api_.EntityModel(ownedPed_)!=kVampireModel)){logger_.Write(util::LogLevel::Warning,"Owned debug vampire handle became invalid or was reused; dropping ownership without deleting it.");ownedPed_=0;state_=State::Idle;}return;}
 if(state_!=State::LoadingModel)return;
 auto status=modelRequest_.Update(api_,frame.nowMs);
 if(status==game::ModelStreamStatus::TimedOut){logger_.Write(util::LogLevel::Error,std::string("cs_vampire model request timed out after ")+std::to_string(frame.nowMs-modelRequest_.StartedAtMs())+" ms.");ResetRequest();return;}
 if(status==game::ModelStreamStatus::Loaded)SpawnLoadedModel(frame.nowMs);
}
bool DebugVampireSpawner::FindSpawnPoint(game::PedHandle player,game::Vec3&position,float&heading)const noexcept{
 if(!api_.PedAlive(player))return false;
 const game::Vec3 playerPos=api_.EntityCoords(player);
 constexpr std::array<game::Vec3,5> offsets{{{0,4,0},{4,0,0},{-4,0,0},{0,-4,0},{0,6,0}}};
 for(const auto&o:offsets){game::Vec3 candidate=api_.OffsetFromEntity(player,o.x,o.y,o.z),safe{};if(!api_.FindSafeCoordForPed(candidate,safe))continue;float d=DistSq(playerPos,safe);if(d<9.0F||d>144.0F||std::fabs(safe.z-playerPos.z)>3.5F)continue;float water=0.0F;if(api_.HasWaterAt(safe,water)&&water>=safe.z-0.25F)continue;position=safe;heading=NormalizeHeading(api_.EntityHeading(player)+180.0F);return true;}
 return false;
}
bool DebugVampireSpawner::SpawnLoadedModel(std::uint64_t nowMs)noexcept{
 auto player=api_.PlayerPed();game::Vec3 point{};float heading=0.0F;
 if(!FindSpawnPoint(player,point,heading)){logger_.Write(util::LogLevel::Error,"No safe nearby spawn coordinate found for cs_vampire.");ResetRequest();return false;}
 auto ped=api_.CreateLocalPed(kVampireModel,point,heading);
 if(ped==0||!api_.EntityExists(ped)||!api_.PedAlive(ped)||api_.EntityModel(ped)!=kVampireModel){if(ped!=0&&api_.EntityExists(ped)&&api_.EntityModel(ped)==kVampireModel)api_.DeletePed(ped);logger_.Write(util::LogLevel::Error,"CREATE_PED failed to produce a valid living cs_vampire ped.");ResetRequest();return false;}
 ownedPed_=ped;state_=State::Spawned;
 auto elapsed=nowMs-modelRequest_.StartedAtMs();modelRequest_.Release(api_);
 logger_.Write(util::LogLevel::Info,std::string("Spawned owned cs_vampire handle=")+std::to_string(ownedPed_)+" modelLoadMs="+std::to_string(elapsed));
 return true;
}
void DebugVampireSpawner::RequestDespawn()noexcept{
 if(state_==State::LoadingModel){modelRequest_.Release(api_);state_=State::Idle;logger_.Write(util::LogLevel::Info,"Cancelled pending cs_vampire model request.");}
 if(ownedPed_==0)return;
 if(!api_.EntityExists(ownedPed_)){ownedPed_=0;state_=State::Idle;return;}
 if(api_.EntityModel(ownedPed_)!=kVampireModel){logger_.Write(util::LogLevel::Error,"Refusing to delete stale owned handle because its model no longer matches cs_vampire.");ownedPed_=0;state_=State::Idle;return;}
 game::PedHandle handle=ownedPed_;
 if(api_.DeletePed(handle)){logger_.Write(util::LogLevel::Info,std::string("Cleaned up owned debug vampire handle=")+std::to_string(ownedPed_));ownedPed_=0;state_=State::Idle;}
 else{ownedPed_=handle;logger_.Write(util::LogLevel::Error,std::string("Failed to delete owned debug vampire handle=")+std::to_string(ownedPed_));}
}
void DebugVampireSpawner::ResetRequest()noexcept{modelRequest_.Release(api_);if(state_!=State::Spawned)state_=State::Idle;}
void DebugVampireSpawner::Cancel()noexcept{RequestDespawn();}
void DebugVampireSpawner::Shutdown()noexcept{RequestDespawn();ResetRequest();}
}
