#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

#include "nightwalker/core/Config.h"
#include "nightwalker/systems/MotionImpulseMath.h"

namespace {
bool Near(float a,float b,float eps=0.0001F){return std::abs(a-b)<=eps;}
}

int main(){
 using nightwalker::game::Vec3;
 using namespace nightwalker::systems::motion_impulse_math;

 {
  const Plan p=BuildPlan(Vec3{0,0,0},Vec3{1,0,0},1.5,0.25,2.0);
  assert(p.valid);assert(Near(p.impulse.x,1.5F));assert(Near(p.impulse.y,0.0F));assert(Near(p.impulse.z,0.25F));
  assert(Near(p.projectedEnd.x,3.0F));assert(Near(p.projectedEnd.y,0.0F));
 }
 {
  const Plan p=BuildPlan(Vec3{0,0,0},Vec3{1,1,0},2.0,0.5,1.0);
  assert(p.valid);const float expected=static_cast<float>(2.0/std::sqrt(2.0));
  assert(Near(p.impulse.x,expected));assert(Near(p.impulse.y,expected));assert(Near(p.impulse.z,0.5F));
 }
 {
  assert(!BuildPlan(Vec3{1,1,0},Vec3{1,1,0},1.0,0.2,2.0).valid);
  const Plan clamped=BuildPlan(Vec3{0,0,0},Vec3{1,0,0},99.0,99.0,99.0);
  assert(clamped.valid);assert(Near(clamped.impulse.x,3.0F));assert(Near(clamped.impulse.z,1.0F));assert(Near(clamped.projectedEnd.x,4.5F));
  assert(WithinRange(Vec3{0,0,0},Vec3{2,0,0},2.0));assert(!WithinRange(Vec3{0,0,0},Vec3{2.1F,0,0},2.0));
 }
 {
  const std::string ini=R"ini(
[Combat]
Enabled=false
MaxDistance=99
HeavyWindupMs=1
ShadowstepFollowupWindupMs=9999
StrikeWindowMs=1
RecoveryMs=1
GrabAlignMs=1
GrabHoldMs=9999
BiteHoldMs=1
ThrowRagdollMs=1
StateTimeoutMs=1
StrikeBonus=999
BiteDamage=17
BiteHeal=9
BiteBloodGain=11.5
StrikeMoveRate=9
ThrowHorizontalForce=9
ThrowUpForce=9
ThrowProjectionMeters=9
BossSpecialCooldownMs=1
)ini";
  auto c=nightwalker::core::Config::Parse(ini);
  assert(!c.IsFeatureEnabled(nightwalker::core::Feature::Combat));
  assert(c.combat.maxDistance==3.0);
  assert(c.combat.heavyWindupMs==120);
  assert(c.combat.shadowstepFollowupWindupMs==500);
  assert(c.combat.strikeWindowMs==250);
  assert(c.combat.recoveryMs==200);
  assert(c.combat.grabAlignMs==200);
  assert(c.combat.grabHoldMs==700);
  assert(c.combat.feedHoldMs==250);
  assert(c.combat.throwRagdollMs==500);
  assert(c.combat.strikeBonus==40);
  assert(c.combat.combatFeedCost==17);
  assert(c.combat.combatFeedRestore==9);
  assert(std::abs(c.combat.combatFeedBloodGain-11.5)<0.0001);
  assert(c.combat.strikeMoveRate==1.12);
  assert(c.combat.throwHorizontalForce==2.5);
  assert(c.combat.throwUpForce==0.75);
  assert(c.combat.throwProjectionMeters==3.5);
  assert(c.combat.bossSpecialCooldownMs==1500);
  assert(c.combat.stateTimeoutMs>=750);
 }
 std::cout<<"Nightwalker Phase 8 combat tests passed\n";
 return 0;
}
