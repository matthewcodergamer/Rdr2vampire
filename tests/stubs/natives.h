#pragma once
#include <cstdint>

using Entity = int;
using Ped = int;
using Hash = std::uint32_t;
using BOOL = int;
inline constexpr BOOL TRUE = 1;
inline constexpr BOOL FALSE = 0;

namespace ENTITY {
bool DOES_ENTITY_EXIST(Entity entity);
void SET_ENTITY_ALPHA(Entity entity, int alphaLevel, BOOL skin);
void SET_ENTITY_VISIBLE(Entity entity, BOOL visible);
void RESET_ENTITY_ALPHA(Entity entity);
}

namespace PAD {
BOOL IS_CONTROL_PRESSED(int padIndex, Hash control);
}

namespace MISC {
Hash GET_HASH_KEY(const char* text);
}

namespace STREAMING {
void REQUEST_NAMED_PTFX_ASSET(Hash asset);
BOOL HAS_NAMED_PTFX_ASSET_LOADED(Hash asset);
void REMOVE_NAMED_PTFX_ASSET(Hash asset);
}

namespace GRAPHICS {
void USE_PARTICLE_FX_ASSET(const char* assetName);
void SET_PARTICLE_FX_NON_LOOPED_COLOUR(float r, float g, float b);
BOOL START_PARTICLE_FX_NON_LOOPED_AT_COORD(
    const char* effectName,
    float xPos, float yPos, float zPos,
    float xRot, float yRot, float zRot,
    float scale,
    BOOL xAxis, BOOL yAxis, BOOL zAxis);
}
