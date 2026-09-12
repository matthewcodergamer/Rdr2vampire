#include "nightwalker/game/GamePresentationApi.h"
#include <natives.h>
namespace nightwalker::game {
namespace {
constexpr Hash kMeleeControl = 0xB2F377E8;
constexpr const char* kSmokeAsset = "scr_fme_spawn_effects";
constexpr const char* kSmokeEffect = "scr_fme_smoke_puff_tint";
Hash SmokeAssetHash() noexcept { return MISC::GET_HASH_KEY(kSmokeAsset); }
}
bool GamePresentationApi::SetPedVisible(PedHandle ped,bool visible) noexcept {
 if(ped==0||!ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped)))return false;
 ENTITY::SET_ENTITY_ALPHA(static_cast<Entity>(ped),visible?255:0,FALSE);
 ENTITY::SET_ENTITY_VISIBLE(static_cast<Entity>(ped),visible?TRUE:FALSE);
 return true;
}
void GamePresentationApi::RestorePedAppearance(PedHandle ped) noexcept {
 if(ped==0||!ENTITY::DOES_ENTITY_EXIST(static_cast<Entity>(ped)))return;
 ENTITY::SET_ENTITY_VISIBLE(static_cast<Entity>(ped),TRUE);
 ENTITY::RESET_ENTITY_ALPHA(static_cast<Entity>(ped));
}
bool GamePresentationApi::MeleeInputPressed() const noexcept {
 return PAD::IS_CONTROL_PRESSED(0,kMeleeControl)==TRUE;
}
bool GamePresentationApi::PulseMeleeInput() noexcept { return false; }
void GamePresentationApi::RequestShadowSmoke() noexcept { const Hash h=SmokeAssetHash();if(!STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(h))STREAMING::REQUEST_NAMED_PTFX_ASSET(h); }
bool GamePresentationApi::PlayShadowSmoke(const Vec3& p,float scale) noexcept {
 const Hash h=SmokeAssetHash();
 if(!STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(h))return false;
 GRAPHICS::USE_PARTICLE_FX_ASSET(kSmokeAsset);
 GRAPHICS::SET_PARTICLE_FX_NON_LOOPED_COLOUR(0.12F,0.12F,0.12F);
 return GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD(kSmokeEffect,p.x,p.y,p.z,0.0F,0.0F,0.0F,scale,FALSE,FALSE,FALSE)!=0;
}
void GamePresentationApi::ReleaseShadowSmoke() noexcept {
 const Hash h=SmokeAssetHash(); if(STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(h))STREAMING::REMOVE_NAMED_PTFX_ASSET(h);
}
}
