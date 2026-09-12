#include "ConfigInternal.h"
#include <algorithm>
#include <string>

namespace nightwalker::core::config_internal {
namespace {
constexpr int kDebugHeavyKey=0x70;
constexpr int kDebugGrabKey=0x71;
constexpr int kDebugReleaseKey=0x72;
constexpr int kDebugCombatFeedKey=0x73;
constexpr int kDebugSipKey=0x74;
constexpr int kDebugDrainKey=0x75;
constexpr int kDebugSpawnKey=0x77;
constexpr int kDebugDespawnKey=0x78;
constexpr int kDefaultShadowstepKey=0x76;
constexpr int kDefaultReloadKey=0x79;
constexpr int kDefaultRestoreKey=0x7A;

template<class T>void Clamp(T& v,T lo,T hi,const char* name,const Config::DiagnosticSink& d){const T before=v;v=std::clamp(v,lo,hi);if(v!=before&&d)d(std::string(name)+" was outside the safe range and was clamped.");}
bool DebugHotkeysCollide(const DebugSettings& d)noexcept{
 if(d.shadowstepHotkey==d.reloadHotkey||d.shadowstepHotkey==d.restoreHotkey||d.reloadHotkey==d.restoreHotkey)return true;
 const int reserved[]={kDebugHeavyKey,kDebugGrabKey,kDebugReleaseKey,kDebugCombatFeedKey,kDebugSipKey,kDebugDrainKey,kDebugSpawnKey,kDebugDespawnKey};
 for(int key:reserved)if(d.shadowstepHotkey==key||d.reloadHotkey==key||d.restoreHotkey==key)return true;
 return false;
}
}

void Validate(Config& c,const Config::DiagnosticSink& d){
 Clamp(c.shadowstep.quickDistance,1.0,15.0,"Shadowstep.QuickDistance",d);Clamp(c.shadowstep.aimDistance,1.0,25.0,"Shadowstep.AimDistance",d);Clamp(c.shadowstep.cooldownMs,100,10000,"Shadowstep.CooldownMs",d);Clamp(c.shadowstep.maxVerticalDelta,0.25,3.0,"Shadowstep.MaxVerticalDelta",d);Clamp(c.shadowstep.validationTimeoutMs,50,2000,"Shadowstep.ValidationTimeoutMs",d);Clamp(c.shadowstep.wallClearance,0.40,1.50,"Shadowstep.WallClearance",d);
 Clamp(c.movement.sprintMoveRate,1.0,1.20,"Movement.SprintMoveRate",d);Clamp(c.movement.accelerationMs,100,1200,"Movement.AccelerationMs",d);Clamp(c.movement.burstDurationMs,300,4000,"Movement.BurstDurationMs",d);Clamp(c.movement.recoveryMs,250,5000,"Movement.RecoveryMs",d);Clamp(c.movement.dismountRecoveryMs,150,2000,"Movement.DismountRecoveryMs",d);Clamp(c.movement.activationDistance,2.5,15.0,"Movement.ActivationDistance",d);Clamp(c.movement.minVelocity,0.05,3.0,"Movement.MinVelocity",d);Clamp(c.movement.trailIntervalMs,100,1000,"Movement.TrailIntervalMs",d);
 Clamp(c.feeding.initialBlood,0.0,100.0,"Feeding.InitialBlood",d);Clamp(c.feeding.sipBloodGain,0.0,100.0,"Feeding.SipBloodGain",d);Clamp(c.feeding.drainBloodGain,0.0,100.0,"Feeding.DrainBloodGain",d);Clamp(c.feeding.healthRestoreSip,0,200,"Feeding.HealthRestoreSip",d);Clamp(c.feeding.healthRestoreDrain,0,300,"Feeding.HealthRestoreDrain",d);Clamp(c.feeding.maxDistance,1.0,2.5,"Feeding.MaxDistance",d);Clamp(c.feeding.alignMs,100,1200,"Feeding.AlignMs",d);Clamp(c.feeding.grabMs,100,1500,"Feeding.GrabMs",d);Clamp(c.feeding.sipDurationMs,300,3000,"Feeding.SipDurationMs",d);Clamp(c.feeding.drainDurationMs,500,4500,"Feeding.DrainDurationMs",d);Clamp(c.feeding.releaseMs,0,1000,"Feeding.ReleaseMs",d);Clamp(c.feeding.stateTimeoutMs,500,8000,"Feeding.StateTimeoutMs",d);
 const int longestFeed=std::max(c.feeding.sipDurationMs,c.feeding.drainDurationMs);if(c.feeding.stateTimeoutMs<longestFeed+250){c.feeding.stateTimeoutMs=std::min(8000,longestFeed+250);if(d)d("Feeding.StateTimeoutMs was raised above the longest feed stage.");}

 Clamp(c.combat.maxDistance,1.25,3.0,"Combat.MaxDistance",d);Clamp(c.combat.heavyWindupMs,120,900,"Combat.HeavyWindupMs",d);Clamp(c.combat.shadowstepFollowupWindupMs,0,500,"Combat.ShadowstepFollowupWindupMs",d);Clamp(c.combat.strikeWindowMs,250,1500,"Combat.StrikeWindowMs",d);Clamp(c.combat.recoveryMs,200,2500,"Combat.RecoveryMs",d);Clamp(c.combat.grabAlignMs,100,900,"Combat.GrabAlignMs",d);Clamp(c.combat.grabHoldMs,200,1200,"Combat.GrabHoldMs",d);Clamp(c.combat.feedHoldMs,250,1200,"Combat.CombatFeedHoldMs",d);Clamp(c.combat.throwRagdollMs,500,2500,"Combat.ThrowRagdollMs",d);Clamp(c.combat.stateTimeoutMs,1200,7000,"Combat.StateTimeoutMs",d);Clamp(c.combat.strikeBonus,0,40,"Combat.StrikeBonus",d);Clamp(c.combat.combatFeedCost,0,60,"Combat.CombatFeedCost",d);Clamp(c.combat.combatFeedRestore,0,60,"Combat.CombatFeedRestore",d);Clamp(c.combat.combatFeedBloodGain,0.0,50.0,"Combat.CombatFeedBloodGain",d);Clamp(c.combat.strikeMoveRate,1.0,1.12,"Combat.StrikeMoveRate",d);Clamp(c.combat.throwHorizontalForce,0.25,2.5,"Combat.ThrowHorizontalForce",d);Clamp(c.combat.throwUpForce,0.0,0.75,"Combat.ThrowUpForce",d);Clamp(c.combat.throwProjectionMeters,0.75,3.5,"Combat.ThrowProjectionMeters",d);Clamp(c.combat.bossSpecialCooldownMs,1500,10000,"Combat.BossSpecialCooldownMs",d);
 const int longestCombatStage=std::max({c.combat.strikeWindowMs,c.combat.grabHoldMs,c.combat.feedHoldMs});if(c.combat.stateTimeoutMs<longestCombatStage+500){c.combat.stateTimeoutMs=std::min(7000,longestCombatStage+500);if(d)d("Combat.StateTimeoutMs was raised above the longest combat stage.");}

 Clamp(c.encounter.startHour,0,23,"Encounter.StartHour",d);Clamp(c.encounter.endHour,0,23,"Encounter.EndHour",d);Clamp(c.encounter.respawnCooldownHours,1,720,"Encounter.RespawnCooldownHours",d);
 Clamp(c.vampireAi.shadowstepMinDistance,2.5,8.0,"VampireAI.ShadowstepMinDistance",d);Clamp(c.vampireAi.shadowstepMaxDistance,5.0,15.0,"VampireAI.ShadowstepMaxDistance",d);Clamp(c.vampireAi.strikingRange,1.2,2.5,"VampireAI.StrikingRange",d);Clamp(c.vampireAi.predictionMs,50,600,"VampireAI.PredictionMs",d);Clamp(c.vampireAi.decisionIntervalMs,80,1000,"VampireAI.DecisionIntervalMs",d);Clamp(c.vampireAi.shadowstepCooldownMs,1200,10000,"VampireAI.ShadowstepCooldownMs",d);Clamp(c.vampireAi.telegraphMs,180,900,"VampireAI.TelegraphMs",d);Clamp(c.vampireAi.recoveryMs,300,3000,"VampireAI.RecoveryMs",d);Clamp(c.vampireAi.evadeCooldownMs,2500,15000,"VampireAI.EvadeCooldownMs",d);Clamp(c.vampireAi.retreatSpeedThreshold,0.2,3.0,"VampireAI.RetreatSpeedThreshold",d);
 if(c.vampireAi.shadowstepMaxDistance<c.vampireAi.shadowstepMinDistance+0.5){c.vampireAi.shadowstepMaxDistance=c.vampireAi.shadowstepMinDistance+0.5;if(d)d("VampireAI.ShadowstepMaxDistance was raised above the minimum distance.");}
 Clamp(c.bossHud.idleSeconds,1.0,30.0,"BossHUD.IdleSeconds",d);Clamp(c.bossHud.fadeSeconds,0.1,3.0,"BossHUD.FadeSeconds",d);Clamp(c.bossHud.deathHoldSeconds,0.0,5.0,"BossHUD.DeathHoldSeconds",d);
 if(DebugHotkeysCollide(c.debug)){if(d)d("Debug hotkeys collided with F1-F6/F8/F9 or each other; restoring safe F7/F10/F11 defaults.");c.debug.shadowstepHotkey=kDefaultShadowstepKey;c.debug.reloadHotkey=kDefaultReloadKey;c.debug.restoreHotkey=kDefaultRestoreKey;}
 if(c.bossHud.showNumericHealth){if(d)d("BossHUD.ShowNumericHealth is locked off by DESIGN_LOCKS.md.");c.bossHud.showNumericHealth=false;}
}

} // namespace nightwalker::core::config_internal
