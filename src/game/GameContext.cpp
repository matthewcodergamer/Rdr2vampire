#include "nightwalker/game/GameContext.h"
#include <natives.h>
#include <utility>
namespace nightwalker::game {
bool GameContext::Initialize(std::filesystem::path p){Reset();if(p.empty())return false;modulePath_=std::move(p);pluginDirectory_=modulePath_.parent_path();return !pluginDirectory_.empty();}
void GameContext::Reset(){modulePath_.clear();pluginDirectory_.clear();}
RuntimeState GameContext::QueryRuntimeState()const noexcept{RuntimeState s{};const auto player=PLAYER::PLAYER_ID();const auto ped=PLAYER::PLAYER_PED_ID();s.playerValid=ped!=0&&ENTITY::DOES_ENTITY_EXIST(ped);s.playerAlive=s.playerValid&&!ENTITY::IS_ENTITY_DEAD(ped);s.controlOn=PLAYER::IS_PLAYER_CONTROL_ON(player);s.missionActive=MISC::GET_MISSION_FLAG();return s;}
}
