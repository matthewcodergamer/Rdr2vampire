#pragma once
#include <cstdint>

using Entity = int;
using Ped = int;
using Player = int;
using Hash = std::uint32_t;
using BOOL = int;
struct Vector3 { float x; float y; float z; };
inline constexpr BOOL TRUE = 1;
inline constexpr BOOL FALSE = 0;

namespace ENTITY {
bool DOES_ENTITY_EXIST(Entity entity);
BOOL IS_ENTITY_A_MISSION_ENTITY(Entity entity);
int GET_ENTITY_HEALTH(Entity entity);
void _SET_ENTITY_HEALTH(Entity entity, int health, int p2);
BOOL HAS_ENTITY_CLEAR_LOS_TO_ENTITY(Entity entity1, Entity entity2, int traceType);
void SET_ENTITY_ALPHA(Entity entity, int alphaLevel, BOOL skin);
void SET_ENTITY_VISIBLE(Entity entity, BOOL visible);
void RESET_ENTITY_ALPHA(Entity entity);
Vector3 GET_ENTITY_VELOCITY(Entity entity, int p1);
Ped GET_PED_INDEX_FROM_ENTITY_INDEX(Entity entity);
}

namespace PLAYER {
Player PLAYER_ID();
BOOL GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(Player player, Entity* entity);
}

namespace PED {
BOOL IS_PED_IN_COMBAT(Ped ped, Ped target);
BOOL IS_PED_IN_MELEE_COMBAT(Ped ped);
BOOL IS_PED_HUMAN(Ped ped);
BOOL IS_PED_IN_ANY_VEHICLE(Ped ped, BOOL atGetIn);
BOOL IS_PED_USING_ANY_SCENARIO(Ped ped);
int GET_PED_MAX_HEALTH(Ped ped);
void SET_PED_MOVE_RATE_OVERRIDE(Ped ped, float value);
BOOL IS_PED_SWIMMING(Ped ped);
BOOL IS_PED_RAGDOLL(Ped ped);
BOOL IS_PED_FALLING(Ped ped);
BOOL IS_PED_ON_MOUNT(Ped ped);
}

namespace TASK {
void TASK_STAND_STILL(Ped ped, int time);
void TASK_COMBAT_PED(Ped ped, Ped targetPed, int p2, int p3);
void TASK_TURN_PED_TO_FACE_ENTITY(Ped ped, Entity targetEntity, int duration, float p3, float p4, float p5);
BOOL TASK_GRAPPLE(Ped attacker, Ped target, Hash p2, BOOL p3, float speed, BOOL p5, Hash p6);
void CLEAR_PED_TASKS(Ped ped, BOOL p1, BOOL p2);
}

namespace PAD { BOOL IS_CONTROL_PRESSED(int padIndex, Hash control); }
namespace MISC { Hash GET_HASH_KEY(const char* text); }
namespace STREAMING { void REQUEST_NAMED_PTFX_ASSET(Hash asset); BOOL HAS_NAMED_PTFX_ASSET_LOADED(Hash asset); void REMOVE_NAMED_PTFX_ASSET(Hash asset); }
namespace GRAPHICS {
void USE_PARTICLE_FX_ASSET(const char* assetName);
void SET_PARTICLE_FX_NON_LOOPED_COLOUR(float r, float g, float b);
BOOL START_PARTICLE_FX_NON_LOOPED_AT_COORD(const char* effectName,float xPos,float yPos,float zPos,float xRot,float yRot,float zRot,float scale,BOOL xAxis,BOOL yAxis,BOOL zAxis);
}
